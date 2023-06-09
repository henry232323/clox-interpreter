#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"
#include "libc/list.h"

static uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t) key[i];
        hash *= 16777619;
    }
    return hash;
}

Obj *allocateObject(size_t size, ObjType type) {
    Obj *object = (Obj *) reallocate(NULL, 0, size);
    object->type = type;
    object->isMarked = false;
    object->next = vm.objects;
    vm.objects = object;

#ifdef DEBUG_LOG_GC
    printf("%p allocate %zu for object %d\n", (void *) object, size, type);
#endif

    return object;
}

static ObjString *allocateString(char *chars, int length,
                                 uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.strings, string, NIL_VAL);
    pop();

#ifdef DEBUG_LOG_GC
    printf("%p allocate string %s\n", string, chars);
#endif

    return string;
}

static ObjAtom *allocateAtom(char *chars, int length,
                                 uint32_t hash) {
    ObjString *string = ALLOCATE_OBJ(ObjString, OBJ_ATOM);
    string->length = length;
    string->chars = chars;
    string->hash = hash;

    push(OBJ_VAL(string));
    tableSet(&vm.atoms, string, NIL_VAL);
    pop();

    return (ObjAtom *) string;
}

ObjString *copyString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) return interned;

    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}

ObjAtom *copyAtom(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.atoms, chars, length,
                                          hash);
    if (interned != NULL) return interned;

    char *heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    return allocateAtom(heapChars, length, hash);
}

ObjUpvalue *newUpvalue(Value *slot) {
    ObjUpvalue *upvalue = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
    upvalue->location = slot;
    upvalue->next = NULL;
    upvalue->closed = NIL_VAL;
    return upvalue;
}

static void printFunction(ObjFunction *function) {
    if (function->name == NULL) {
        printf("<script>");
        return;
    }

    printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_FUNCTION:
            printFunction(AS_FUNCTION(value));
            break;
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_ATOM:
            printf(":%s", AS_CSTRING(value));
            break;
        case OBJ_NATIVE_METHOD:
            printf("<native method>");
            break;
        case OBJ_NATIVE:
            printf("<native fn %p>", AS_NATIVE(value));
            break;
        case OBJ_CALL_FRAME:
            printf("<callframe %p>", AS_CALL_FRAME(value));
            break;
        case OBJ_CLOSURE:
            printFunction(AS_CLOSURE(value)->function);
            break;
        case OBJ_UPVALUE:
            printf("upvalue");
            break;
        case OBJ_BUILTIN_TYPE: {
            ObjClass *klass = AS_CLASS(value);
            printf("<builtin type ");
            printf(klass->name->chars, strlen(klass->name->chars));
            printf(">");
            break;
        }
        case OBJ_CLASS:
            printf("<type %s>", AS_CLASS(value)->name->chars);
            break;
        case OBJ_LIST:
        case OBJ_MAP:
        case OBJ_INSTANCE: {
            ObjInstance *instance = AS_INSTANCE(value);
            ObjType objType = instance->klass->obj.type;
            switch (objType) {
                case OBJ_CLASS: {
                    printf("<%s instance>",
                           AS_INSTANCE(value)->klass->name->chars);
                    break;
                }
                case OBJ_BUILTIN_TYPE: {
                    ObjBuiltinType *type = (ObjBuiltinType *)AS_INSTANCE(value)->klass;
                    type->printFn((Obj*) instance);
                    break;
                }
                default: {

                }
            }
            break;
        }
        case OBJ_BOUND_METHOD: {
            ObjBoundMethod * boundMethod = AS_BOUND_METHOD(value);

            switch (boundMethod->method->obj.type) {
                case OBJ_CLOSURE:
                    printFunction(boundMethod->method->function);
                    break;
                case OBJ_NATIVE_METHOD:
                    printf("<builtin method>");
                    break;
            }
            break;
        }
        default: {
            printf("<unknown %d>", OBJ_TYPE(value));
        }
    }
}

ObjString *takeString(char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = tableFindString(&vm.strings, chars, length,
                                          hash);
    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    return allocateString(chars, length, hash);
}

ObjFunction *newFunction() {
    ObjFunction *function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
    function->arity = 0;
    function->upvalueCount = 0;
    function->name = NULL;
    initChunk(&function->chunk);
    return function;
}

ObjInstance *newInstance(ObjClass *klass) {
    ObjInstance *instance = ALLOCATE_OBJ(ObjInstance, OBJ_INSTANCE);
    instance->klass = klass;
    copyTable(&klass->fields, &instance->fields);

    return instance;
}

ObjBoundMethod* newBoundMethod(Value receiver,
                               ObjClosure* method) {
    ObjBoundMethod* bound = ALLOCATE_OBJ(ObjBoundMethod,
                                         OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClosure *newClosure(ObjFunction *function) {
    ObjUpvalue **upvalues = ALLOCATE(ObjUpvalue*,
                                     function->upvalueCount);
    for (int i = 0; i < function->upvalueCount; i++) {
        upvalues[i] = NULL;
    }

    ObjClosure *closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalueCount = function->upvalueCount;
    return closure;
}

ObjNative *newNative(NativeFn function) {
    ObjNative *native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
    native->function = function;
    return native;
}

ObjClass *newClass(ObjString *name) {
    ObjClass *klass = ALLOCATE_OBJ(ObjClass, OBJ_CLASS);
    klass->name = name;
    initTable(&klass->methods);
    initTable(&klass->fields);
    return klass;
}
#ifndef CRAFTING_INTERPRETERS_LIST_H
#define CRAFTING_INTERPRETERS_LIST_H

#include "../object.h"
#include "type.h"

#define AS_LIST(value) ((ObjList *)AS_OBJ(value))

typedef struct {
    ObjInstance obj;
    ValueArray items;
} ObjList;

ObjList *newList();

void freeList(ObjList *list);

void markList(ObjList *list);

void printList(ObjList *list);

Value listCall(int argCount, Value *args);

void listPush(ObjList *list, Value item);

Value getLength(ObjList *list, int argCount, Value *args);

ObjBuiltinType *createListType();


#endif //CRAFTING_INTERPRETERS_LIST_H

/* 
 * File:   newClass.h
 * Author: ywhwang
 *
 * Created on 2013? 9? 13? (?), ?? 4:49
 */

#ifndef NEWCLASS_H
#define	NEWCLASS_H
#include"eloop_callback_itf.h"
class newClass {
public:
    newClass();
    newClass(const newClass& orig);
    virtual ~newClass();
private:

};
class newTaskClass : public TaskCallbackInterface{
	PbaseTaskCtx ctx2;
public:
	void* Run(int arg1);
	void* Run2(int arg1);
	newTaskClass();
    newTaskClass(int startTask,int stkSize=DEFAULT_TASK_STACK_SIZE,int arg1=0,int arg2=0);
    newTaskClass(const newTaskClass& orig);
    virtual ~newTaskClass();
private:

};

#endif	/* NEWCLASS_H */


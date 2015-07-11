/*------------------------------------------------------------------*//*{{{*/
/* Copyright (C) SSE-USTC, 2014-2015                                */
/*                                                                  */
/*  FILE NAME             :  native.c                               */
/*  LANGUAGE              :  C                                      */
/*  TARGET ENVIRONMENT    :  ANY                                    */
/*  DATE OF FIRST RELEASE :  2015/03/06                             */
/*  DESCRIPTION           :  for the javaVM                         */
/*------------------------------------------------------------------*/

/*
 * Revision log:
 *
 *//*}}}*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../main/vm.h"
#include "../util/control.h"
#include "../util/string.h"
#include "../classloader/resolve.h"
#include "../classloader/class.h"
#include "../util/exception.h"
#include "../lib/linkedlist.h"
#include "../util/testvm.h"
#include "../util/string.h"
#include <sys/utsname.h>
#include "reflect.h"


#define THROW throwException("native")
// just for test============
void testObject();         
//============================

void nativeWriteBuf();
void identityHashCode();
void constructNative();
void getDeclaredConstructors();
void forName();
void getClass();
void printInt();
void printChar();
void printLong();
void printString();
void printDouble();
void printFloat();
void printBoolean();
void printObject0();
void newline();
void getPrimitiveClass();
void getClassLoader0();
void desiredAssertionStatus0();
void fillInStackTrace();
void getClass();
void getName0();
void registerNatives();
void floatToRawIntBits();
void doubleToRawLongBits();
void longBitsToDouble();

void insertSystemProperties();
void arrayCopy();
void currentClassLoader();
void nativeGetLibname();
void nativeLoad();
void isWordsBidEndian();
void turkeyCopy();
void nativeInit();
void nativeValid();

extern Frame* current_frame;
extern NativeFrame* nframe;
extern LinkedList* head;



/* Binding method name with function*/
Binding nativeMethods[] = {
    {"nativeWriteBuf", "(J[BII)J", nativeWriteBuf},
    {"identityHashCode", "(Ljava/lang/Object;)I", identityHashCode},
    {"constructNative", "([Ljava/lang/Object;Ljava/lang/Class;I)Ljava/lang/Object;", constructNative},
    {"getDeclaredConstructors", "(Z)[Ljava/lang/reflect/Constructor;",getDeclaredConstructors},
    {"forName", "(Ljava/lang/String;)Ljava/lang/Class;", forName},
    {"getClass", "()Ljava/lang/Class;", getClass},
    {"testObject", "(Ljava/lang/Object;)V", testObject},
    {"nativeValid", "(J)Z", nativeValid},
    {"nativeInit", "()V", nativeInit},
    {"clone", "(Ljava/lang/Cloneable;)Ljava/lang/Object;", turkeyCopy},
    {"isWordsBigEndian", "()Z", isWordsBidEndian},
    {"nativeLoad", "(Ljava/lang/String;)I", nativeLoad},
    {"nativeGetLibname", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", nativeGetLibname},
    {"currentClassLoader", "()Ljava/lang/ClassLoader;", currentClassLoader},
    {"arraycopy", "(Ljava/lang/Object;ILjava/lang/Object;II)V", arrayCopy},
    {"insertSystemProperties", "(Ljava/util/Properties;)V", insertSystemProperties},
    //{"longBitsToDouble", "(J)D", longBitsToDouble},
    //{"doubleToRawLongBits", "(D)J", doubleToRawLongBits},
    //{"floatToRawIntBits", "(F)I", floatToRawIntBits},
    //{"getName0", "()Ljava/lang/String;", getName0},
    //{"getClass", "()Ljava/lang/Class;", getClass},
    //{"fillInStackTrace", "(I)Ljava/lang/Throwable;", fillInStackTrace},
    //{"desiredAssertionStatus0", "(Ljava/lang/Class;)Z", desiredAssertionStatus0},
    //{"getClassLoader0", "()Ljava/lang/ClassLoader;", getClassLoader0},
    {"registerNatives", "()V", registerNatives},
    //{"getPrimitiveClass", "(Ljava/lang/String;)Ljava/lang/Class;", getPrimitiveClass},
    {"printf", "(I)V", printInt},
    {"printf", "(C)V", printChar},
    {"printf", "(J)V", printLong},
    {"printf", "(D)V", printDouble},
    {"printf", "(F)V", printFloat},
    {"printf", "(Z)V", printBoolean},
    {"printf", "(Ljava/lang/Object;)V", printObject0},
    {"printf", "(Ljava/lang/String;)V", printString},
    {"newline", "()V", newline},
    {NULL, NULL, NULL},
};


/*FileDescriptor*/
void nativeWriteBuf() {
    long long fd = *(long long*)&nframe->locals[1];
    Object* buf = *(Object**)&nframe->locals[3];
    int _offset = nframe->locals[4];
    int len = nframe->locals[5];

    if (!buf->isArray)
      throwException("not array");
    char* p = (char*)buf->data;
    int i;
    for (i=_offset; i<len; i++) {
        printf("%c", *p);
        p++;
    }

    current_frame->ostack++;
    *(long long*)current_frame->ostack = 1;
}

/*VMClass.class*/
void identityHashCode(){
    Object* this = *(Object**)&nframe->locals[0];

    current_frame->ostack++;
    *(int*)current_frame->ostack = this;


}

static void invoke(MethodBlock* mb, Object* args, Object* this) {
    //printf("<invoke>\n");
    unsigned short max_stack = mb->max_stack;
    unsigned short max_locals = mb->max_locals;
    int args_count = mb->args_count;
    int locals_idx = 0;
    Class* class = mb->class;
    ClassBlock* cb = CLASS_CB(class);


    Frame* frame = (Frame*)sysMalloc(sizeof(Frame));
    frame->mb = mb;
    frame->cp = &cb->constant_pool;
    frame->pc = mb->code;
    frame->class = class;
    frame->locals = (unsigned int*)sysMalloc(
                sizeof(int) * (max_stack + max_locals));
    //set 0
    memset(frame->locals, 0, sizeof(int)*(max_stack+max_locals));
    //point to the previous slot of aviliable
    frame->ostack = frame->locals + max_locals - 1;

    //copy args
    /*
     * old_stack                 new_locals
     * --------                   ---------
     * |objref|                [0]|       |
     * --------                   ---------
     * |arg1  |                [1]|       |
     * --------                   ---------<-locals_idx(static)
     * |arg2  |                [2]|       |
     * --------<-top              ---------<-locals_idx(non-static)
     *
     * @qcliu 2015/01/29
     */

    if (args->isArray != 1) {
        throwException("args must be array");
    }
    //copyArgs(Frame* frame, MethodBlock* mb)
    if (!(mb->access_flags & ACC_STATIC))//non-static
      locals_idx = args_count;
    else
      throwException("construct method must be non-static");

    if (args->length != locals_idx) {
        throwException("args count error");
    }

    *((Object**)&frame->locals[0]) = this;
    int i;
    for (i = 0; i<args->length; i++) {
        *((Object**)&frame->locals[i+1]) = ARRAY_DATA(args, i, Object*);

        //NOTE: equals 0 also need copy
        //memcpy(frame->locals + locals_idx, current_frame->ostack, sizeof(int));
        /*NOTE: pop the stack*/
        //*current_frame->ostack = 0;
        //current_frame->ostack--;
    }
    //point to the prev
    frame->prev = current_frame;
    current_frame = frame;


    executeJava();

    Frame* temp = current_frame;
    current_frame = current_frame->prev;
    free(temp->locals);
    free(temp);
}


/**
 * @see java/lang/Constructor.class
 *
 */
void constructNative() {


    Object* this = *(Object**)&nframe->locals[0];

    //a array
    Object* args = *(Object**)&nframe->locals[1];
    // 
    Object* declaringClass = *(Object**)&nframe->locals[2];
    // 
    int slot = nframe->locals[3];

    Class* declclass = declaringClass->binding;
    ClassBlock* cb = CLASS_CB(declclass);
    MethodBlock* mb = &cb->methods[slot];
    //char* method_name = mb->name;
    //char* method_type = mb->type;
    
    Object* newobj = (Object*)allocObject(declclass);
    Object* newobj1 = (Object*)allocObject(declclass);
    Object* newobj2 = (Object*)allocObject(declclass);
    Object* newobj3 = (Object*)allocObject(declclass);
    Object* newobj4 = (Object*)allocObject(declclass);
    Object* newobj5 = (Object*)allocObject(declclass);
    Object* newobj6 = (Object*)allocObject(declclass);

    //2015/07/01
    //DEBUG("TODO");
    //exit(0);

    //TODO automatically unwrapped and widened, if needed
    
    //@TEST
    //printObjectWrapper(args);
    //Object* inputstream = ARRAY_DATA(args, 0, Object*);

    //executeMethodArgs(NULL, mb, newobj,inputstream);
    
    invoke(mb, args, newobj);
    current_frame->ostack++;
    *(Object**)current_frame->ostack = newobj;



}

void getDeclaredConstructors() {
    Object* vmClass = *(Object**)&nframe->locals[0];
    int isPublic = nframe->locals[1];

    Object* cons = getClassConstructors(vmClass, isPublic);

    current_frame->ostack++;
    *(Object**)current_frame->ostack = cons;


}

//lang/lang/VMClass.class
void forName() {
    Object* name = *(Object**)&nframe->locals[0];

    //printString0(name);

    char* classname = String2Char(name);
    //printf("%s\n", classname);

    int len = strlen(classname);
    int i=0;
    for (; i<len; i++) {
        if (classname[i] == '.')
          classname[i] = '/';
    }

    Class* class = (Class*)loadClass(classname);
    ClassBlock* cb = CLASS_CB(class);

    if (class == NULL)
      throwException("NoSuchClass");

    Object* cobj = class->class;
    //Class* c = cobj->class;
    //cb = CLASS_CB(c);

    current_frame->ostack++;
    *(Object**)current_frame->ostack = cobj;

}

/**
 * @parm classname
 *      like Ljava/io/InputStream
 *
 * @return a class object for this classname
 */
Object* getClass_name(char* classname) {
    int len = strlen(classname);
    int i = 0;
    for (; i<len; i++) {
        if (classname[i] == '.')
          classname[i] = '/';
    }

    Class* class = (Class*)loadClass(classname);
    ClassBlock* cb = CLASS_CB(class);
    if (class == NULL)
      throwException("NoSuchClass");


    Object* cobj = class->class;

    return cobj;
}

//VMSystem.class
/**
 * this is an extra method, only for test.
 */
void testObject() {
    Object* obj = *(Object**)&nframe->locals[0];
    if (obj == NULL) {
        printf("obj == null, in testObject");
        throwException("hash false");
    }
    else {
        //printString0(obj);
        throwException("hash success");
    }

}

//java/lang/Class
/*void getClass()
  {
  Object* this = *(Object**)&nframe->locals[0];
  Class* class = this->class;
  Object* class_obj = class->class;

  current_frame->ostack++;
 *(Object**)current_frame->ostack = class_obj;


 }*/

//java/io/FileDescriptor

void nativeValid() {
    int ret = TRUE;
    long long nativeFd = *(long long*)&nframe->locals[1];//not static
    if (nativeFd >= 0)
      ret = TRUE;
    else
      ret = FALSE;

    // long needs 2 slot in stack
    current_frame->ostack++;
    *(int*)current_frame->ostack = ret;

}

void nativeInit() {
    //printf("nativeInit\n");
    Class* c = (Class*)findClassInTable(head, "java/io/FileDescriptor");
    FieldBlock* fb = (FieldBlock*)findField(c, "out", "Ljava/io/FileDescriptor;");
    FieldBlock* fb_err = (FieldBlock*)findField(c, "err", "Ljava/io/FileDescriptor;");
    Object* err = (Object*)fb_err->static_value;
    Object* out = (Object*)fb->static_value;

    //Class* class = out->class;
    //ClassBlock* cb = CLASS_CB(class);
    MethodBlock* mb = (MethodBlock*)findMethod(out->class, "<init>", "(J)V");
    if (mb == NULL)
      throwException("no such method!");
    /*hack*/
    executeMethodArgs(out->class, mb, out, 0, 0);

    MethodBlock* mb_err = (MethodBlock*)findMethod(err->class, "<init>", "(J)V");
    if (mb_err == NULL)
      throwException("no such method!");
    executeMethodArgs(err->class, mb_err, err, 0, 0);
}


//(Ljava/lang/Cloneable;)Ljava/lang/Object;   VMObject
void turkeyCopy() {
    Object* cloneable = *(Object**)&nframe->locals[0];
    int copy_size = cloneable->copy_size;
    Object* obj = (Object*)sysMalloc(copy_size);
    memset(obj, 0, copy_size);
    memcpy(obj, cloneable, copy_size);

    /*NOTE: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    obj->data = (unsigned int*)(obj+1);

    current_frame->ostack++;
    *(Object**)current_frame->ostack = obj;

}

void isWordsBidEndian() {
    current_frame->ostack++;
    *(int*)current_frame->ostack = 0;
}
void nativeLoad(){
    Object* string = *(Object**)&nframe->locals[1];
    char* s = String2Char(string);
    int i = resolveDll(s);
    current_frame->ostack++;
    *(int*)current_frame->ostack = i;
}

void nativeGetLibname() {
    Object* pathname = *(Object**)&nframe->locals[0];
    Object* libname = *(Object**)&nframe->locals[1];
    char* path = String2Char(pathname);
    char* name = String2Char(libname);


    char* lib = getDllName(path, name);
    free(path);
    free(name);

    Object* s = createString(lib);

    current_frame->ostack++;
    *(Object**)current_frame->ostack = s;
    free(lib);
}

void currentClassLoader() {
    current_frame->ostack++;
    *current_frame->ostack = 0;
}



/**
 *
 *
 */
void setProperty(Object* this, char* key, char* value) {
    Object* k = createString(key);
    Object* v = createString(value);
    // char* s = String2Char(k);
    // char* t = String2Char(v);

    MethodBlock* mb = (MethodBlock*)findMethod(this->class, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    if (mb == NULL)
      throwException("no such method");

    executeMethodArgs(this->class, mb,this, k, v);

    /*hack*/
    //NOTE: after executeMethod, stack is not correct, so we need revise it.
    current_frame->ostack--;
}

void arrayCopy() {
    //printf("native arrayCopy----------------------------------------\n");
    int size;
    Object* src = *(Object**)&nframe->locals[0];
    int srcStart = nframe->locals[1];
    Object* dest = *(Object**)&nframe->locals[2];
    int destStart = nframe->locals[3];
    int length = nframe->locals[4];

    if ((src == NULL)||(dest == NULL))
      throwException("java/lang/NullPointerException");
    else {
        ClassBlock* scb = CLASS_CB(src->class);
        ClassBlock* dcb = CLASS_CB(dest->class);

        unsigned int* sdata = src->data;
        unsigned int* ddata = dest->data;

        if ((scb->this_classname[0] != '[') || (dcb->this_classname[0] != '['))
          goto storeExcep;

        size = 0;
        switch (scb->this_classname[1]) {
            case 'B':
                size = 1;
                break;
            case 'C':
            case 'S':
                size = 2;
                break;
            case 'I':
            case 'F':
            case 'L':
            case '[':
            case 'Z':
                size = 4;
                break;
            case 'J':
            case 'D':
                size = 8;
                break;
            default:
                throwException("type error");
        }


        memmove(((char*)&ddata[0] + destStart * size),
                    ((char*)&sdata[0] + srcStart * size),
                    length*size
               );
        //printChar0(dest);
        return;


    }



    goto storeExcep;

    return;

storeExcep:
    throwException("java/lang/ArrayStoreException");
}

void insertSystemProperties() {
    Object*  this = *(Object**)&nframe->locals[0];

    struct utsname info;
    uname(&info);


    setProperty(this, "java.vm.name", "TurkeyVM");
    setProperty(this, "java.vm.version", "1.0");
    setProperty(this, "java.vm.vendor", "Robert Lougher");
    setProperty(this, "java.vm.vendor.url", "http://jamvm.sourceforge.net");
    setProperty(this, "java.version", "1.4");
    setProperty(this, "java.vendor", "GNU Classpath");
    setProperty(this, "java.vendor.url", "http://gnu.classpath.org");
    setProperty(this, "java.home", "qcliu");
    setProperty(this, "java.specification.version", "1.4");
    setProperty(this, "java.specification.vendor", "Sun Microsystems, Inc.");
    setProperty(this, "java.specification.name", "Java Platform API Specification");
    setProperty(this, "java.vm.specification.version", "1.0");
    setProperty(this, "java.vm.specification.vendor", "Sun Microsystems, Inc.");
    setProperty(this, "java.vm.specification.name", "Java Virtual Machine Specification");
    setProperty(this, "java.class.version", "48.0");
    setProperty(this, "java.class.path", "");

    setProperty(this, "java.library.path", getDllPath());

    setProperty(this, "java.io.tmpdir", "/tmp");
    setProperty(this, "java.compiler", "tiger");
    setProperty(this, "java.ext.dirs", "");
    setProperty(this, "os.name", info.sysname);
    setProperty(this, "os.arch", info.machine);
    setProperty(this, "os.version", info.release);
    setProperty(this, "file.separator", "/");
    setProperty(this, "path.separator", ":");
    setProperty(this, "line.separator", "\n");
    setProperty(this, "user.name", getenv("USER"));
    setProperty(this, "user.home", getenv("HOME"));
    setProperty(this, "user.dir", getenv("PWD"));
}

void longBitsToDouble() {
    long long value = *(long long*)&nframe->locals[0];
    double result;
    memcpy(&result, &value, 8);
    current_frame->ostack++;
    *(double*)current_frame->ostack = result;
    current_frame->ostack++;
}

void doubleToRawLongBits() {
    double value = *(double*)&nframe->locals[0];
    current_frame->ostack++;
    *(double*)current_frame->ostack = value;
    current_frame->ostack++;
}

/* java/lang/Float */
void floatToRawIntBits() {
    //static
    float value = *(float*)&nframe->locals[0];
    current_frame->ostack++;
    *(float*)current_frame->ostack = value;

}

/*java/lang/Class*/
void getName0() {
    //This must be a java/lang/Class Object
    Object* obj = (Object*)nframe->locals[0];
    if (obj->binding == NULL)
      throwException("getName0 error; binding NULL");

    Class* class = obj->binding;
    ClassBlock* cb = CLASS_CB(class);
    Object* string = createString(cb->this_classname);
    if (string == NULL)
      throwException("getName0 error. string is NULL");

    current_frame->ostack++;
    *(Object**)current_frame->ostack = string;
}

/*java/lang/Object*/
void getClass() {
    Object* obj = (Object*)nframe->locals[0];
    Class* class = obj->class;
    //ClassBlock* cb = CLASS_CB(class);
    Object* class_obj = class->class;

    current_frame->ostack++;
    *(Object**)current_frame->ostack = class_obj;
}

void fillInStackTrace() {
    Class* class = loadClass("java/lang/Throwable");

    if (class == NULL)
      throwException("no find java/lang/Throwable, in native.c");

    Object* obj = allocObject(class);

    current_frame->ostack++;
    *(Object**)current_frame->ostack = obj;
}
/*(Ljava/lang?Class;)Z
 * This method is belong to java/lang/Class
 * assume that is alway return true;
 * @qcliu 2015/03/21
 */
//TODO
void desiredAssertionStatus0() {
    current_frame->ostack++;
    *current_frame->ostack = 1;
}

/* ()Ljava/lang/ClassLoader;
 * This method is belong to java/lang/Class
 * assume that is alway return 0.
 *
 * @qcliu 2015/03/21
 */
void getClassLoader0() {
    //TODO
    current_frame->ostack++;
    *current_frame->ostack = 0;
}
void registerNatives() {
    ClassBlock* cb = CLASS_CB(current_frame->class);
    //printf("registerNatives, class:%s\n", cb->this_classname);
}


static char getPrimType(char* s) {
    char primtype;
    if (strcmp(s, "byte")==0)
      primtype = 'B';
    else if (strcmp(s, "short") == 0)
      primtype = 'S';
    else if (strcmp(s, "int") == 0)
      primtype = 'I';
    else if (strcmp(s, "long") == 0)
      primtype = 'J';
    else if (strcmp(s, "char") == 0 )
      primtype = 'C';
    else if (strcmp(s, "float") == 0)
      primtype = 'F';
    else if (strcmp(s, "double") == 0)
      primtype = 'D';
    else if (strcmp(s, "boolean") == 0)
      primtype = 'Z';
    else
      throwException("getPrimType error");

    return primtype;
}
void getPrimitiveClass() {
    /*In the frame is a String*/
    Object* obj = (Object*)nframe->locals[0];
    Object* array = (Object*)obj->data[0];

    //printf("getPrimitiveClass:%s\n", (char*)array->data);
    char primtype = getPrimType((char*)array->data);

    Class* class = (Class*)findPrimitiveClass(primtype);
    if (class != NULL) {
        current_frame->ostack++;
        *(Object**)current_frame->ostack = class->class;
    }
    else
      throwException("getPrimitiveClass");
}
void newline() {
    printf("\n");
}

/*
 * String is a CharArray, in it data[0] is a reference to
 * the CharArray.
 * NOTE: printf("%s", (char*)array->data)) instead of 
 *       printf("%s", (char*)array->data[0]);
 */
void printString() {

    Object* obj =(Object*)nframe->locals[1];
    Object* array = (Object*)obj->data[0];
    printf("%s", (char*)array->data);
}

void printObject0() {
    Object* obj = (Object*)nframe->locals[1];
    Class* class = obj->class;
    ClassBlock* cb = CLASS_CB(class);
    printf("%s", cb->this_classname);
}
void printBoolean() {
    int value = *(int*)&nframe->locals[1];
    if(value)
      printf("true");
    else
      printf("false");
}
void printFloat() {
    float value = *(float*)&nframe->locals[1];
    printf("%f", value);
}
void printDouble() {
    double value = *(double*)&nframe->locals[1];
    printf("%f", value);
}

void printLong() {
    long long value =*(long long*)&nframe->locals[1];
    printf("%lld", value);
}

void printChar() {
    char value = (char)nframe->locals[1];
    printf("%c", value);
}

void printInt() {
    MethodBlock* mb = nframe->mb;
    Class* class = mb->class;
    ClassBlock* cc = CLASS_CB(class);

    if (dis_testinfo) {
        printf("current nFrame is :%s, %s  locals:%d, stack:%d\n", 
                    mb->name, mb->type, mb->max_locals, mb->max_stack);
        printf("current class:%s\n", cc->this_classname);
        printf("printInt!!!!\n");
    }
    int value = nframe->locals[1];
    printf("%d", value);
}

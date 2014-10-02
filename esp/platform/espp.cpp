/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#pragma warning (disable : 4786)
#if defined(_WIN32) && defined(_DEBUG)
//#include <vld.h>
#endif
//Jlib
#include "jliball.hpp"

//CRT / OS
#ifndef _WIN32
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif

#include <jni.h>
#include "eclrtl.hpp"

//SCM Interfaces
#include "esp.hpp"

// We can use "work" as the first parameter when debugging.
#define ESP_SINGLE_PROCESS

//ESP Core
#include "espp.hpp"
#include "espcfg.ipp"
#include "esplog.hpp"
#include "espcontext.hpp"
#include "build-config.h"

#ifdef _WIN32
/*******************************************
  _WIN32
 *******************************************/

#ifdef ESP_SINGLE_PROCESS

int start_init_main(int argc, char** argv, int (*init_main_func)(int,char**))
{
    return init_main_func(argc, argv);
}

#define START_WORK_MAIN(config, server, result)  result = work_main((config), (server));
#define SET_ESP_SIGNAL_HANDLER(sig, handler)
#define RESET_ESP_SIGNAL_HANDLER(sig, handler)

#else //!ESP_SINGLE_PROCESS

#define SET_ESP_SIGNAL_HANDLER(sig, handler)
#define RESET_ESP_SIGNAL_HANDLER(sig, handler)

int start_init_main(int argc, char** argv, int (*init_main_func)(int, char**))
{ 
    if(argc > 1 && !strcmp(argv[1], "work")) 
    { 
        char** newargv = new char*[argc - 1]; 
        newargv[0] = argv[0]; 
        for(int i = 2; i < argc; i++) 
        { 
            newargv[i - 1] = argv[i]; 
        } 
        int rtcode = init_main_func(argc - 1, newargv); 
        delete[] newargv;
        return rtcode;
    } 
    else 
    { 
        StringBuffer command; 
        command.append(argv[0]); 
        command.append(" work "); 
        for(int i = 1; i < argc; i++) 
        { 
            command.append(argv[i]); 
            command.append(" "); 
        } 
        DWORD exitcode = 0; 
        while(true) 
        { 
            PROGLOG("Starting working process: %s", command.str()); 
            PROCESS_INFORMATION process; 
            STARTUPINFO si; 
            GetStartupInfo(&si); 
            if(!CreateProcess(NULL, (char*)command.str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &process)) 
            { 
                ERRLOG("Process failed: %d\r\n",GetLastError()); 
                exit(-1); 
            } 
            WaitForSingleObject(process.hProcess,INFINITE); 
            GetExitCodeProcess(process.hProcess, &exitcode); 
            PROGLOG("Working process exited, exitcode=%d", exitcode); 
            if(exitcode == TERMINATE_EXITCODE) 
            { 
                DBGLOG("This is telling the monitoring process to exit too. Exiting once and for all...."); 
                exit(exitcode); 
            } 
            CloseHandle(process.hProcess); 
            CloseHandle(process.hThread); 
            Sleep(1000);
        } 
    }
}

#define START_WORK_MAIN(config, server, result)  result = work_main((config), (server));
#endif //!ESP_SINGLE_PROCESS

#else

/*****************************************************
     LINUX
 ****************************************************/
#define SET_ESP_SIGNAL_HANDLER(sig, handler) signal(sig, handler)
#define RESET_ESP_SIGNAL_HANDLER(sig, handler) signal(sig, handler)

int start_init_main(int argc, char** argv, int (*init_main_func)(int,char**))
{
    return init_main_func(argc, argv);
}

int work_main(CEspConfig& config, CEspServer& server);
int do_work_main(CEspConfig& config, CEspServer& server)
{
   int result; 
   int numchildren = 0; 
   pid_t childpid=0; 

createworker: 
   childpid = fork(); 
   if(childpid < 0) 
   { 
      ERRLOG("Unable to create new process"); 
      result = -1; 
   } 
   else if(childpid == 0) 
   {
        result = work_main(config, server);
   } 
   else 
   { 
      DBGLOG("New process generated, pid=%d", childpid); 
      numchildren++; 
      if(numchildren < MAX_CHILDREN) 
         goto createworker; 
      
      int status; 
      childpid = wait3(&status, 0, NULL); 
      DBGLOG("Attention: child process exited, pid = %d", childpid); 
      numchildren--; 
      DBGLOG("Bringing up a new process..."); 
      sleep(1); 
      goto createworker; 
   }

   return result;
}

#define START_WORK_MAIN(config, server, result) result = do_work_main(config, server)

#endif

void brokenpipe_handler(int sig)
{
   //Reset the signal first
   RESET_ESP_SIGNAL_HANDLER(SIGPIPE, brokenpipe_handler);

   DBGLOG("Broken Pipe - remote side closed the socket");
}


int work_main(CEspConfig& config, CEspServer& server)
{
    server.start();
    DBGLOG("ESP server started.");
    server.waitForExit(config);
    server.stop(true);
    config.stopping();
    config.clear();
    return 0;
}


void openEspLogFile(IPropertyTree* envpt, IPropertyTree* procpt)
{
    StringBuffer logdir;
    if(procpt->hasProp("@name"))
    {
        StringBuffer espNameStr;
        procpt->getProp("@name", espNameStr);
        if (!getConfigurationDirectory(envpt->queryPropTree("Software/Directories"), "log", "esp", espNameStr.str(), logdir))
        {
            logdir.clear();
        }
    }
    if(logdir.length() == 0)
    {
        if(procpt->hasProp("@logDir"))
            procpt->getProp("@logDir", logdir);
    }


    Owned<IComponentLogFileCreator> lf = createComponentLogFileCreator(logdir.str(), "esp");
    lf->setName("esp_main");//override default filename
    lf->setAliasName("esp");
    lf->beginLogging();

    if (procpt->getPropBool("@enableSysLog", false))
        UseSysLogForOperatorMessages();
}   

static void usage()
{
    puts("ESP - Enterprise Service Platform server. (C) 2001-2011, HPCC Systems.");
    puts("Usage:");
    puts("  esp [options]");
    puts("Options:");
    puts("  -?/-h: show this help page");
    puts("  interactive: start in interactive mode (pop up error dialog when exception occurs)");
    puts("  config=<file>: specify the config file name [default: esp.xml]");
    puts("  process=<name>: specify the process name in the config [default: the 1st process]");
    exit(1);
}

static void checkException(JNIEnv *JNIenv)
{
    if (JNIenv->ExceptionCheck())
    {
        jthrowable exception = JNIenv->ExceptionOccurred();
        JNIenv->ExceptionClear();
        jclass throwableClass = JNIenv->FindClass("java/lang/Throwable");
        jmethodID throwableToString = JNIenv->GetMethodID(throwableClass, "toString", "()Ljava/lang/String;");
        jstring cause = (jstring) JNIenv->CallObjectMethod(exception, throwableToString);
        const char *text = JNIenv->GetStringUTFChars(cause, 0);
        VStringBuffer message("javaembed: %s", text);
        JNIenv->ReleaseStringUTFChars(cause, text);
        rtlFail(0, message.str());
    }
}

void fromKELToECL(JNIEnv *JNIenv, const char* kel)
{
    unsigned unicodeChars;
    UChar *unicode;
    PROGLOG("**** fromKELToECL 1");
    rtlStrToUnicodeX(unicodeChars, unicode, strlen(kel), kel);
    PROGLOG("**** fromKELToECL 2");
    jstring jArg = JNIenv->NewString(unicode, unicodeChars);
    PROGLOG("**** fromKELToECL 3");

    checkException(JNIenv);
    JNIenv->ExceptionClear();
    jclass testInputOutputStringClass = JNIenv->FindClass("TestInputOutputString");
    PROGLOG("**** fromKELToECL 4");
    checkException(JNIenv);
    jmethodID getCompileMethod = JNIenv->GetStaticMethodID(testInputOutputStringClass, "AddTestToInput", "(Ljava/lang/String;)Ljava/lang/String;");
    PROGLOG("**** fromKELToECL 5");
    checkException(JNIenv);
    jstring result = (jstring) JNIenv->CallStaticObjectMethod(testInputOutputStringClass, getCompileMethod, jArg);
    PROGLOG("**** fromKELToECL 6");
    checkException(JNIenv);

    StringBuffer ecl, message;
    size_t size = JNIenv->GetStringUTFLength(result);  // in bytes
    PROGLOG("**** fromKELToECL 7");
    const char *text = JNIenv->GetStringUTFChars(result, NULL);
    if (text)
        ecl.set(text);
    PROGLOG("**** fromKELToECL 8");
    message.setf("fromKELToECL: %s, length: %ld", text, size);
    JNIenv->ReleaseStringUTFChars(result, text);
    JNIenv->DeleteLocalRef(result);
    PROGLOG("**** ecl<%s>", ecl.str());
    PROGLOG("**** message<%s>", message.str());
    PROGLOG("**** fromKELToECL OK");
    PROGLOG("**** fromKELToECL 9");
    return;
}

void fromKELToECL2(JNIEnv *JNIenv, const char* kel)
{
    PROGLOG("**** fromKELToECL O1");
    jint iArg = 9;
    JNIenv->ExceptionClear();
    jclass testInputOutputStringClass = JNIenv->FindClass("TestInputOutput");
    PROGLOG("**** fromKELToECL O2");
    checkException(JNIenv);
    jmethodID getCompileMethod = JNIenv->GetStaticMethodID(testInputOutputStringClass, "AddTestToInputObj2", "(LTestDataInput;)LTestDataOutput;");
    PROGLOG("**** fromKELToECL O3");
    checkException(JNIenv);

    jstring inStr1 = JNIenv->NewStringUTF(kel);
    jstring inStr2 = JNIenv->NewStringUTF("5678");
    PROGLOG("**** fromKELToECL O4");

    jclass testInputClass = JNIenv->FindClass("TestDataInput");
    PROGLOG("**** fromKELToECL O5");
    checkException(JNIenv);
    jmethodID constructor = JNIenv->GetMethodID(testInputClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;I)V");
    PROGLOG("**** fromKELToECL O51");
    checkException(JNIenv);

    jobject inObject = JNIenv->NewObject(testInputClass, constructor, inStr1, inStr2, iArg);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O6");

    jobject outObject = JNIenv->CallStaticObjectMethod(testInputOutputStringClass, getCompileMethod, inObject);
    assert(outObject != NULL);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O7");

    jclass outClass = JNIenv->GetObjectClass(outObject);
    jmethodID methodID = JNIenv->GetMethodID(outClass, "getOutStr1","()Ljava/lang/String;");
    jstring outStr1 = (jstring) JNIenv->CallObjectMethod(outObject, methodID);
    PROGLOG("**** fromKELToECL O73");

    //methodID = JNIenv->GetMethodID(outClass, "getOutInt1", "(I)V");
    //jint outInt1 = JNIenv->CallIntMethod(outObject, methodID);

    StringBuffer ecl, message;
    size_t size = JNIenv->GetStringUTFLength(outStr1);  // in bytes
    const char *text = JNIenv->GetStringUTFChars(outStr1, NULL);
    if (text)
        ecl.set(text);
    message.setf("fromKELToECL: %s, length: %ld", text, size);
    PROGLOG("**** ecl %s", ecl.str());
    PROGLOG("**** message %s", message.str());
    JNIenv->ReleaseStringUTFChars(outStr1, text);
    JNIenv->DeleteLocalRef(outStr1);
    PROGLOG("**** fromKELToECL O9");
    return;
}

void fromKELToECL3(JNIEnv *JNIenv, const char* kel)
{
    PROGLOG("**** fromKELToECL O1");
    JNIenv->ExceptionClear();
    jclass testInputOutputStringClass = JNIenv->FindClass("TestInputOutput2");
    PROGLOG("**** fromKELToECL O2");
    checkException(JNIenv);
    jmethodID getCompileMethod = JNIenv->GetStaticMethodID(testInputOutputStringClass, "AddTestToInputObj2", "(LTestDataInput2;)LTestDataOutput2;");
    PROGLOG("**** fromKELToECL O3");
    checkException(JNIenv);

    jstring inStr1 = JNIenv->NewStringUTF(kel);
    jstring inStr2 = JNIenv->NewStringUTF("5678");
    jstring inStr3 = JNIenv->NewStringUTF("99");
    PROGLOG("**** fromKELToECL O4");

    jclass testInputClass = JNIenv->FindClass("TestDataInput2");
    PROGLOG("**** fromKELToECL O5");
    checkException(JNIenv);
    jmethodID constructor = JNIenv->GetMethodID(testInputClass, "<init>", "([Ljava/lang/String;Ljava/lang/String;)V");
    PROGLOG("**** fromKELToECL O51");
    checkException(JNIenv);

    jclass classString = JNIenv->FindClass("java/lang/String");
    jobjectArray outJNIArray = JNIenv->NewObjectArray(2, classString, NULL);

    jmethodID midStringInit = JNIenv->GetMethodID(classString, "<init>", "(Ljava/lang/String;)V");
    //if (NULL == midStringInit) return NULL;
    jobject objStr1 = JNIenv->NewObject(classString, midStringInit, inStr1);
    jobject objStr2 = JNIenv->NewObject(classString, midStringInit, inStr2);
    JNIenv->SetObjectArrayElement(outJNIArray, 0, objStr1);
    JNIenv->SetObjectArrayElement(outJNIArray, 1, objStr2);

    jobject inObject = JNIenv->NewObject(testInputClass, constructor, outJNIArray, inStr3);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O6");

    jobject outObject = JNIenv->CallStaticObjectMethod(testInputOutputStringClass, getCompileMethod, inObject);
    assert(outObject != NULL);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O7");

    jclass outClass = JNIenv->GetObjectClass(outObject);
    jmethodID methodID = JNIenv->GetMethodID(outClass, "getOutStr","()Ljava/lang/String;");
    jstring outStr = (jstring) JNIenv->CallObjectMethod(outObject, methodID);
    PROGLOG("**** fromKELToECL O73");

    jmethodID methodID2 = JNIenv->GetMethodID(outClass, "getOutStrs","()[Ljava/lang/String;");
    jobjectArray outStrs = (jobjectArray) JNIenv->CallObjectMethod(outObject, methodID2);

    const char *param[20];
    jsize stringCount = JNIenv->GetArrayLength(outStrs);

    for (unsigned i=0; i<stringCount; i++)
    {
        jstring string = (jstring) JNIenv->GetObjectArrayElement( outStrs, i);
        param[i] = JNIenv->GetStringUTFChars( string, NULL);
        PROGLOG("**** param %s", param[i]);
    }
    PROGLOG("**** fromKELToECL O73");

    StringBuffer ecl, message;
    size_t size = JNIenv->GetStringUTFLength(outStr);  // in bytes
    const char *text = JNIenv->GetStringUTFChars(outStr, NULL);
    if (text)
        ecl.set(text);
    message.setf("fromKELToECL: %s, length: %ld", text, size);
    PROGLOG("**** ecl %s", ecl.str());
    PROGLOG("**** message %s", message.str());
    JNIenv->ReleaseStringUTFChars(outStr, text);
    JNIenv->DeleteLocalRef(outStr);
    PROGLOG("**** fromKELToECL O9");
    return;
}

void fromKELToECL4(JNIEnv *JNIenv, const char* kel)
{
    PROGLOG("**** fromKELToECL O1 *********");
    JNIenv->ExceptionClear();
    jclass testInputClass = JNIenv->FindClass("org/hpccsystems/kel/CompilerInputs");
    PROGLOG("**** fromKELToECL O5");
    checkException(JNIenv);
    jclass testInputOutputStringClass = JNIenv->FindClass("org/hpccsystems/kel/Main");
    PROGLOG("**** fromKELToECL O2");
    checkException(JNIenv);
    jmethodID getCompileMethod = JNIenv->GetStaticMethodID(testInputOutputStringClass, "compile", "(Lorg/hpccsystems/kel/CompilerInputs;)Lorg/hpccsystems/kel/CompilerOutputs;");
    PROGLOG("**** fromKELToECL O3");
    checkException(JNIenv);

    jstring inStr0 = JNIenv->NewStringUTF(kel);
    jstring inStr1 = JNIenv->NewStringUTF("-o");
    jstring inStr2 = JNIenv->NewStringUTF("first_test1.ecl");
    jstring inStr3 = JNIenv->NewStringUTF("first_test.kel");
    PROGLOG("**** fromKELToECL O4");

    jmethodID constructor = JNIenv->GetMethodID(testInputClass, "<init>", "([Ljava/lang/String;Ljava/lang/String;)V");
    PROGLOG("**** fromKELToECL O51");
    checkException(JNIenv);

    jclass classString = JNIenv->FindClass("java/lang/String");
    jobjectArray inJNIArray = JNIenv->NewObjectArray(3, classString, NULL);

    jmethodID midStringInit = JNIenv->GetMethodID(classString, "<init>", "(Ljava/lang/String;)V");
    //if (NULL == midStringInit) return NULL;
    jobject objStr1 = JNIenv->NewObject(classString, midStringInit, inStr1);
    jobject objStr2 = JNIenv->NewObject(classString, midStringInit, inStr2);
    jobject objStr3 = JNIenv->NewObject(classString, midStringInit, inStr3);
    JNIenv->SetObjectArrayElement(inJNIArray, 0, objStr1);
    JNIenv->SetObjectArrayElement(inJNIArray, 1, objStr2);
    JNIenv->SetObjectArrayElement(inJNIArray, 2, objStr3);

    jobject inObject = JNIenv->NewObject(testInputClass, constructor, inJNIArray, inStr0);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O6");

    jobject outObject = JNIenv->CallStaticObjectMethod(testInputOutputStringClass, getCompileMethod, inObject);
    assert(outObject != NULL);
    checkException(JNIenv);
    PROGLOG("**** fromKELToECL O7");

    jclass outClass = JNIenv->GetObjectClass(outObject);
    jmethodID methodID = JNIenv->GetMethodID(outClass, "getOutput","()Ljava/lang/String;");
    jstring outStr = (jstring) JNIenv->CallObjectMethod(outObject, methodID);
    PROGLOG("**** fromKELToECL O71");

    StringBuffer ecl, message;
    size_t size = JNIenv->GetStringUTFLength(outStr);  // in bytes
    PROGLOG("**** fromKELToECL O711");
    if (size > 0)
    {
        PROGLOG("**** fromKELToECL O712");
		const char *text = JNIenv->GetStringUTFChars(outStr, NULL);
		if (text)
		{
		    PROGLOG("**** fromKELToECL O713");
			ecl.set(text);
			message.setf("fromKELToECL: %s, length: %ld", text, size);
			PROGLOG("**** ecl %s", ecl.str());
			PROGLOG("**** message %s", message.str());
		    JNIenv->ReleaseStringUTFChars(outStr, text);
		}
    }
    PROGLOG("**** fromKELToECL O714");
    JNIenv->DeleteLocalRef(outStr);

    jmethodID methodID2 = JNIenv->GetMethodID(outClass, "getErrors","()[Ljava/lang/String;");
    PROGLOG("**** fromKELToECL O72");
    jobjectArray outStrs = (jobjectArray) JNIenv->CallObjectMethod(outObject, methodID2);
    PROGLOG("**** fromKELToECL O73");

    if (outStrs != NULL)
    {
		const char *param[20];
		jsize stringCount = JNIenv->GetArrayLength(outStrs);
		PROGLOG("**** fromKELToECL O74");
		for (unsigned i=0; i<stringCount; i++)
		{
			PROGLOG("**** fromKELToECL O731");
			jstring string = (jstring) JNIenv->GetObjectArrayElement( outStrs, i);
			param[i] = JNIenv->GetStringUTFChars( string, NULL);
			PROGLOG("**** param %s", param[i]);
		}
    }
    PROGLOG("**** fromKELToECL O9");
    return;
}
/*
void fromKELToECL(JNIEnv *JNIenv, const char* kel,  StringBuffer& ecl, StringBuffer& message)
{
const char[] rawData = {0,1,2,3,4,5,6,7,8,9}; //Or get some raw data from somewhere
int dataSize = sizeof(rawData);
printf("Building raw data array copy\n");
jbyteArray rawDataCopy = env->NewByteArray(dataSize);
env->SetByteArrayRegion(rawDataCopy, 0, dataSize, rawData);


printf("Finding callback method\n");
//Assumes obj is the Java instance that will receive the raw data via callback
jmethodID aMethodId = env->GetMethodID(env->GetObjectClass(obj),"handleData","([B)V");
if(0==aMethodId) throw MyRuntimeException("Method not found error");
printf("Invoking the callback\n");
env->CallVoidMethod(obj,aMethodId, &rawDataCopy);

}*/

void test_jni()
{
    StringArray optionStrings;
    optionStrings.append("-Djava.compiler=NONE");           //disable JIT
#ifdef SIMPLE_TEST
    optionStrings.append("-Djava.class.path=/home/lexis/hpcc/test/kel"); // user classes
#else
    optionStrings.append("-Djava.class.path=/opt/HPCCSystems/5.0.2/KEL:/opt/HPCCSystems/5.0.2/KEL/KEL.jar"); // user classes
    optionStrings.append("-Djava.library.path=/opt/HPCCSystems/5.0.2/KEL");
#endif
    //optionStrings.append("-Djava.library.path=/usr/lib/jvm/java-1.7.0-openjdk-amd64/jre/lib/amd64/server");  // set native library path
    //optionStrings.append("-verbose:jni");                   // print JNI-related messages

    JavaVMOption* options = new JavaVMOption[optionStrings.length()];
    ForEachItemIn(idx, optionStrings)
    {
        options[idx].optionString = (char *) optionStrings.item(idx);
//        options[idx].extraInfo = NULL;
    }

    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_1_6;
    vm_args.options = options;
    vm_args.nOptions = optionStrings.length();
    vm_args.ignoreUnrecognized = TRUE;

    JavaVM *javaVM;
    JNIEnv *env;
    int createResult = JNI_CreateJavaVM(&javaVM, (void**)&env, &vm_args);
    delete [] options;

    if (createResult != 0)
    {
        PROGLOG("Unable to initialize JVM: JNI_CreateJavaVM returns %d", createResult);
        return;
    }

    //fromKELToECL(env, "1234");
    //fromKELToECL2(env, "1234");

    fromKELToECL3(env, "1234");

    StringBuffer inString;
    inString.append("Person := ENTITY( FLAT(UID=DID,First_Name=fname,Last_Name=lname,Middle_Name=mname) );\n");
    inString.append("Company := ENTITY( FLAT(UID=BDID,Company_Name=company_name) );\n");
    inString.append("KnownLink := ASSOCIATION( FLAT(Person Who,Company What) );\n");
    inString.append("USE Header.File_Headers(FLAT,Person),\n");
    inString.append("Business_Header.File_Header(FLAT,Company),\n");
    inString.append("Business_Header.File_Business_Contacts(FLAT,KnownLink);\n");
    inString.append("QUERY: FindPeopleInContext(_First_Name,_Last_Name) <= Person(_First_Name,_Last_Name), Person(_First_Name), Person(_Last_Name);\n");
    //inString.append("\n");
    fromKELToECL4(env, inString.str());
}

int init_main(int argc, char* argv[])
{
    InitModuleObjects();

    Owned<IProperties> inputs = createProperties(true);

    bool interactive = false;

    for (int i = 1; i < argc; i++)
    {
        if (stricmp(argv[i], "-?")==0 || stricmp(argv[i], "-h")==0 || stricmp(argv[i], "-help")==0
             || stricmp(argv[i], "/?")==0 || stricmp(argv[i], "/h")==0)
             usage();
        else if(stricmp(argv[i], "interactive") == 0)
            interactive = true;
        else if (strchr(argv[i],'='))
        {
            inputs->loadProp(argv[i]);
        }
        else
        {
            fprintf(stderr, "Unknown option: %s", argv[i]);
            return 0;
        }
    }

    int result = -1;

#ifdef _WIN32 
    if (!interactive)
        ::SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
#endif

    SET_ESP_SIGNAL_HANDLER(SIGPIPE, brokenpipe_handler);

    bool SEHMappingEnabled = false;

    CEspAbortHandler abortHandler;

    Owned<IFile> sentinelFile = createSentinelTarget();
    removeSentinelFile(sentinelFile);

    Owned<CEspConfig> config;
    Owned<CEspServer> server;
    try
    {
        const char* cfgfile = NULL;
        const char* procname = NULL;
        if(inputs.get())
        {
            if(inputs->hasProp("config"))
                cfgfile = inputs->queryProp("config");
            if(inputs->hasProp("process"))
                procname = inputs->queryProp("process");
        }
        if(!cfgfile || !*cfgfile)
            cfgfile = "esp.xml";

        Owned<IPropertyTree> envpt= createPTreeFromXMLFile(cfgfile, ipt_caseInsensitive);
        Owned<IPropertyTree> procpt = NULL;
        if (envpt)
        {
            envpt->addProp("@config", cfgfile);
            StringBuffer xpath;
            if (procname==NULL || strcmp(procname, ".")==0)
                xpath.appendf("Software/EspProcess[1]");
            else
                xpath.appendf("Software/EspProcess[@name=\"%s\"]", procname);

            DBGLOG("Using ESP configuration section [%s]", xpath.str());
            procpt.set(envpt->queryPropTree(xpath.str()));
            if (!procpt)
                throw MakeStringException(-1, "Config section [%s] not found", xpath.str());
        }
        else
            throw MakeStringException(-1, "Failed to load config file %s", cfgfile);

        const char* build_ver = BUILD_TAG;
        setBuildVersion(build_ver);

        const char* build_level = BUILD_LEVEL;
        setBuildLevel(build_level);

        openEspLogFile(envpt.get(), procpt.get());

        DBGLOG("Esp starting %s", BUILD_TAG);

test_jni();

        StringBuffer componentfilesDir;
        if(procpt->hasProp("@componentfilesDir"))
            procpt->getProp("@componentfilesDir", componentfilesDir);
        if(componentfilesDir.length() > 0 && strcmp(componentfilesDir.str(), ".") != 0)
        {
            DBGLOG("componentfiles are under %s", componentfilesDir.str());
            setCFD(componentfilesDir.str());
        }

        StringBuffer sehsetting;
        procpt->getProp("@enableSEHMapping", sehsetting);
        if(!interactive && sehsetting.length() > 0 && (stricmp(sehsetting.str(), "true") == 0 || stricmp(sehsetting.str(), "1") == 0))
            SEHMappingEnabled = true;
        if(SEHMappingEnabled)
            EnableSEHtoExceptionMapping();

        CEspConfig* cfg = new CEspConfig(inputs.getLink(), envpt.getLink(), procpt.getLink(), false);
        if(cfg && cfg->isValid())
        {
            config.setown(cfg);
            abortHandler.setConfig(cfg);
        }
    }
    catch(IException* e)
    {
        StringBuffer description;
        ERRLOG("ESP Unhandled IException (%d -- %s)", e->errorCode(), e->errorMessage(description).str());
        e->Release();
        return -1;
    }
    catch (...)
    {
        ERRLOG("ESP Unhandled General Exception.");
        return -1;
    }

    if (config && config->isValid())
    {
        PROGLOG("Configuring Esp Platform...");

        try
        {
            CEspServer *srv = new CEspServer(config);
            if(SEHMappingEnabled)
                srv->setSavedSEHHandler(SEHMappingEnabled);
            server.setown(srv);
            abortHandler.setServer(srv);
            setEspContainer(server.get());

            config->loadAll();
            config->bindServer(*server.get(), *server.get()); 
            
        }
        catch(IException* e)
        {
            StringBuffer description;
            ERRLOG("ESP Unhandled IException (%d -- %s)", e->errorCode(), e->errorMessage(description).str());
            e->Release();
            return -1;
        }
        catch (...)
        {
            ERRLOG("ESP Unhandled General Exception.");
            return -1;
        }

        writeSentinelFile(sentinelFile);
        result = work_main(*config, *server.get());
    }
    else
    {
        ERRLOG("!!! Unable to load ESP configuration.");
    }
    
    return result;
}

//command line arguments:
// [pre] if "work", special init behavior, but removed before init_main  
// [1] process name
// [2] config location - local file name or dali address
// [3] config location type - "dali" or ""

int main(int argc, char* argv[])
{
    start_init_main(argc, argv, init_main);
    stopPerformanceMonitor();
    UseSysLogForOperatorMessages(false);
    releaseAtoms();
    return 0;
}


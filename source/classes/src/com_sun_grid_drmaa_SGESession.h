/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_sun_grid_drmaa_SGESession */

#ifndef _Included_com_sun_grid_drmaa_SGESession
#define _Included_com_sun_grid_drmaa_SGESession
#ifdef __cplusplus
extern "C" {
#endif
#undef com_sun_grid_drmaa_SGESession_SUSPEND
#define com_sun_grid_drmaa_SGESession_SUSPEND 0L
#undef com_sun_grid_drmaa_SGESession_RESUME
#define com_sun_grid_drmaa_SGESession_RESUME 1L
#undef com_sun_grid_drmaa_SGESession_HOLD
#define com_sun_grid_drmaa_SGESession_HOLD 2L
#undef com_sun_grid_drmaa_SGESession_RELEASE
#define com_sun_grid_drmaa_SGESession_RELEASE 3L
#undef com_sun_grid_drmaa_SGESession_TERMINATE
#define com_sun_grid_drmaa_SGESession_TERMINATE 4L
/* Inaccessible static: JOB_IDS_SESSION_ALL */
#undef com_sun_grid_drmaa_SGESession_TIMEOUT_WAIT_FOREVER
#define com_sun_grid_drmaa_SGESession_TIMEOUT_WAIT_FOREVER -1LL
#undef com_sun_grid_drmaa_SGESession_TIMEOUT_NO_WAIT
#define com_sun_grid_drmaa_SGESession_TIMEOUT_NO_WAIT 0LL
#undef com_sun_grid_drmaa_SGESession_UNDETERMINED
#define com_sun_grid_drmaa_SGESession_UNDETERMINED 0L
#undef com_sun_grid_drmaa_SGESession_QUEUED_ACTIVE
#define com_sun_grid_drmaa_SGESession_QUEUED_ACTIVE 16L
#undef com_sun_grid_drmaa_SGESession_SYSTEM_ON_HOLD
#define com_sun_grid_drmaa_SGESession_SYSTEM_ON_HOLD 17L
#undef com_sun_grid_drmaa_SGESession_USER_ON_HOLD
#define com_sun_grid_drmaa_SGESession_USER_ON_HOLD 18L
#undef com_sun_grid_drmaa_SGESession_USER_SYSTEM_ON_HOLD
#define com_sun_grid_drmaa_SGESession_USER_SYSTEM_ON_HOLD 19L
#undef com_sun_grid_drmaa_SGESession_RUNNING
#define com_sun_grid_drmaa_SGESession_RUNNING 32L
#undef com_sun_grid_drmaa_SGESession_SYSTEM_SUSPENDED
#define com_sun_grid_drmaa_SGESession_SYSTEM_SUSPENDED 33L
#undef com_sun_grid_drmaa_SGESession_USER_SUSPENDED
#define com_sun_grid_drmaa_SGESession_USER_SUSPENDED 34L
#undef com_sun_grid_drmaa_SGESession_DONE
#define com_sun_grid_drmaa_SGESession_DONE 48L
#undef com_sun_grid_drmaa_SGESession_FAILED
#define com_sun_grid_drmaa_SGESession_FAILED 64L
/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeControl
 * Signature: (Ljava/lang/String;I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeControl
  (JNIEnv *, jobject, jstring, jint);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeExit
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeExit
  (JNIEnv *, jobject);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeGetContact
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetContact
  (JNIEnv *, jobject);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeGetDRMSInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetDRMSInfo
  (JNIEnv *, jobject);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeGetJobProgramStatus
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetJobProgramStatus
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeInit
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeInit
  (JNIEnv *, jobject, jstring);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeRunBulkJobs
 * Signature: (IIII)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeRunBulkJobs
  (JNIEnv *, jobject, jint, jint, jint, jint);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeRunJob
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_sun_grid_drmaa_SGESession_nativeRunJob
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeSynchronize
 * Signature: ([Ljava/lang/String;JZ)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSynchronize
  (JNIEnv *, jobject, jobjectArray, jlong, jboolean);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeWait
 * Signature: (Ljava/lang/String;J)Lcom/sun/grid/drmaa/SGEJobInfo;
 */
JNIEXPORT jobject JNICALL Java_com_sun_grid_drmaa_SGESession_nativeWait
  (JNIEnv *, jobject, jstring, jlong);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeAllocateJobTemplate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sun_grid_drmaa_SGESession_nativeAllocateJobTemplate
  (JNIEnv *, jobject);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeSetAttributeValue
 * Signature: (ILjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValue
  (JNIEnv *, jobject, jint, jstring, jstring);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeSetAttributeValues
 * Signature: (ILjava/lang/String;[Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeSetAttributeValues
  (JNIEnv *, jobject, jint, jstring, jobjectArray);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeGetAttributeNames
 * Signature: (I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttributeNames
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeGetAttribute
 * Signature: (ILjava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_com_sun_grid_drmaa_SGESession_nativeGetAttribute
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     com_sun_grid_drmaa_SGESession
 * Method:    nativeDeleteJobTemplate
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sun_grid_drmaa_SGESession_nativeDeleteJobTemplate
  (JNIEnv *, jobject, jint);

#ifdef __cplusplus
}
#endif
#endif
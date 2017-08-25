#include <jni.h>
#include <string>
#include <compile.h>

#include "opencv2/core/core_c.h"

#include "dmz.h"

#include "scan/scan.h"

static dmz_context *dmz = NULL;
static int dmz_refcount = 0;

static ScannerState scannerState;
static bool flipped;

static struct {
  jclass classRef;
  jfieldID top;
  jfieldID bottom;
  jfieldID left;
  jfieldID right;
} rectId;

static struct {
  jclass classRef;
  jfieldID complete;
  jfieldID topEdge;
  jfieldID bottomEdge;
  jfieldID leftEdge;
  jfieldID rightEdge;
  jfieldID focusScore;
  jfieldID prediction;
  jfieldID expiry_month;
  jfieldID expiry_year;
  jfieldID detectedCard;
} detectionInfoId;

static struct {
  jclass classRef;
  jmethodID edgeUpdateCallback;
} cardScannerId;

void updateEdgeDetectDisplay(JNIEnv *env,
                             jobject thiz,
                             jobject dinfo,
                             dmz_edges found_edges) {
    env->SetBooleanField(dinfo, detectionInfoId.topEdge, found_edges.top.found);
    env->SetBooleanField(dinfo, detectionInfoId.bottomEdge, found_edges.bottom.found);
    env->SetBooleanField(dinfo, detectionInfoId.leftEdge, found_edges.left.found);
    env->SetBooleanField(dinfo, detectionInfoId.rightEdge, found_edges.right.found);

    env->CallVoidMethod(thiz, cardScannerId.edgeUpdateCallback, dinfo);
}

extern "C"
JNIEXPORT jfloat JNICALL
nCalculateFocusScore(JNIEnv *env, jobject thiz, jbyteArray jb, jint width, jint height) {
    IplImage *image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 1);
    jbyte *jBytes = env->GetByteArrayElements(jb, 0);
    image->imageData = (char *) jBytes;

    float focusScore = dmz_focus_score(image, false);
    //float focusScore = 2.0;

    cvReleaseImageHeader(&image);
    env->ReleaseByteArrayElements(jb, jBytes, 0);

    return focusScore;
}

extern "C"
JNIEXPORT jboolean JNICALL
nDetectEdges(JNIEnv *env,
             jobject thiz,
             jbyteArray jbY,
             jbyteArray jbU,
             jbyteArray jbV,
             jint orientation,
             jint width,
             jint height,
             jobject dinfo) {

    IplImage *image = cvCreateImageHeader(cvSize(width, height), IPL_DEPTH_8U, 1);
    jbyte *jBytesY = env->GetByteArrayElements(jbY, 0);
    image->imageData = (char *) jBytesY;

    IplImage *cb = cvCreateImageHeader(cvSize(width / 2, height / 2), IPL_DEPTH_8U, 1);
    jbyte *jBytesU = env->GetByteArrayElements(jbU, 0);
    cb->imageData = (char *) jBytesU;

    IplImage *cr = cvCreateImageHeader(cvSize(width / 2, height / 2), IPL_DEPTH_8U, 1);
    jbyte *jBytesV = env->GetByteArrayElements(jbV, 0);
    cr->imageData = (char *) jBytesV;

    dmz_edges found_edges;
    dmz_corner_points corner_points;
    bool cardDetected = dmz_detect_edges(image, cb, cr,
                                         orientation,
                                         &found_edges, &corner_points
    );

    updateEdgeDetectDisplay(env, thiz, dinfo, found_edges);

    return (jboolean) cardDetected;
}

extern "C"
JNIEXPORT void JNICALL
nGetGuideFrame(JNIEnv *env, jobject thiz, jint orientation, jint width, jint height, jobject rect) {
    dmz_trace_log("Java_io_card_payment_CardScanner_nGetGuideFrame");

    dmz_rect dr = dmz_guide_frame(orientation, width, height);

    env->SetIntField(rect, rectId.top, dr.y);
    env->SetIntField(rect, rectId.left, dr.x);
    env->SetIntField(rect, rectId.bottom, dr.y + dr.h);
    env->SetIntField(rect, rectId.right, dr.x + dr.w);
}

extern "C"
JNIEXPORT void JNICALL
nSetup(JNIEnv *env, jobject thiz) {
    dmz_debug_log("Java_example_com_br_detectcardnumber_DetectEdges_nSetup");
    dmz_trace_log("dmz trace enabled");

    flipped = false;

    if (dmz == NULL) {
        dmz = dmz_context_create();
        scanner_initialize(&scannerState);
    } else {
        scanner_reset(&scannerState);
    }
    dmz_refcount++;

    cvSetErrMode(CV_ErrModeParent);
}

static JNINativeMethod methodsFocusScore[] = {
    {"nCalculateFocusScore", "([BII)F", (void *) nCalculateFocusScore}
};

static JNINativeMethod methodsDetectEdges[] = {
    {"nDetectEdges", "([B[B[BIIILexample/com/br/detectcardnumber/cardio/DetectionInfo;)Z",
     (void *) nDetectEdges},
    {"nGetGuideFrame", "(IIILandroid/graphics/Rect;)V", (void *) nGetGuideFrame},
    {"nSetup", "()V", (void *) nSetup}
};

static JNINativeMethod methodsTransform3D[] = {
    {"nDetectEdges", "([B[B[BIIILexample/com/br/detectcardnumber/cardio/DetectionInfo;)Z",
     (void *) nDetectEdges},
    {"nGetGuideFrame", "(IIILandroid/graphics/Rect;)V", (void *) nGetGuideFrame},
    {"nSetup", "()V", (void *) nSetup}
};

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    jint status = vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status != JNI_OK)
        return -1;

    jclass myClass = env->FindClass("example/com/br/detectcardnumber/Transform3D");
    if (!myClass) {
        dmz_error_log("Couldn't find CardScanner from JNI");
        return -1;
    }
    cardScannerId.classRef = (jclass) env->NewGlobalRef(myClass);
    cardScannerId.edgeUpdateCallback = env->GetMethodID(myClass,
                                                        "onEdgeUpdate",
                                                        "(Lexample/com/br/detectcardnumber/cardio/DetectionInfo;)V");
    if (!cardScannerId.edgeUpdateCallback) {
        dmz_error_log("Couldn't find edge update callback");
        return -1;
    }

    jclass rectClass = env->FindClass("android/graphics/Rect");
    if (!rectClass) {
        dmz_error_log("Couldn't find Rect class");
        return -1;
    }
    rectId.classRef = (jclass) env->NewGlobalRef(rectClass);
    rectId.top = env->GetFieldID(rectClass, "top", "I");
    rectId.bottom = env->GetFieldID(rectClass, "bottom", "I");
    rectId.left = env->GetFieldID(rectClass, "left", "I");
    rectId.right = env->GetFieldID(rectClass, "right", "I");

    if (!(rectId.top && rectId.bottom && rectId.left && rectId.right)) {
        dmz_error_log("Couldn't find square class");
        return -1;
    }

    jclass dInfoClass = env->FindClass("example/com/br/detectcardnumber/cardio/DetectionInfo");
    if (dInfoClass == NULL) {
        dmz_error_log("Couldn't find DetectionInfo class");
        return -1;
    }
    detectionInfoId.classRef = (jclass) env->NewGlobalRef(dInfoClass);
    detectionInfoId.complete = env->GetFieldID(dInfoClass, "complete", "Z");
    detectionInfoId.topEdge = env->GetFieldID(dInfoClass, "topEdge", "Z");
    detectionInfoId.bottomEdge = env->GetFieldID(dInfoClass, "bottomEdge", "Z");
    detectionInfoId.leftEdge = env->GetFieldID(dInfoClass, "leftEdge", "Z");
    detectionInfoId.rightEdge = env->GetFieldID(dInfoClass, "rightEdge", "Z");
    detectionInfoId.focusScore = env->GetFieldID(dInfoClass, "focusScore", "F");
    detectionInfoId.prediction = env->GetFieldID(dInfoClass, "prediction", "[I");
    detectionInfoId.expiry_month = env->GetFieldID(dInfoClass, "expiry_month", "I");
    detectionInfoId.expiry_year = env->GetFieldID(dInfoClass, "expiry_year", "I");
    detectionInfoId.detectedCard = env->GetFieldID(dInfoClass,
                                                   "detectedCard",
                                                   "Lexample/com/br/detectcardnumber/cardio/CreditCard;");

    if (!(detectionInfoId.complete && detectionInfoId.topEdge && detectionInfoId.bottomEdge
        && detectionInfoId.leftEdge && detectionInfoId.rightEdge
        && detectionInfoId.focusScore && detectionInfoId.prediction
        && detectionInfoId.expiry_month && detectionInfoId.expiry_year
        && detectionInfoId.detectedCard
    )) {
        dmz_error_log("at least one field was not found for DetectionInfo");
        return -1;
    }

    jclass FocusScoreActivity = env->FindClass("example/com/br/detectcardnumber/FocusScore");
    if (FocusScoreActivity == NULL)
        abort();

    env->RegisterNatives(FocusScoreActivity, methodsFocusScore, 1);

    jclass DetectEdgesActivity = env->FindClass("example/com/br/detectcardnumber/DetectEdges");
    if (DetectEdgesActivity == NULL)
        abort();

    env->RegisterNatives(DetectEdgesActivity, methodsDetectEdges, 3);

    jclass Transform3DActivity = env->FindClass("example/com/br/detectcardnumber/Transform3D");
    if (Transform3DActivity == NULL)
        abort();

    env->RegisterNatives(Transform3DActivity, methodsTransform3D, 3);

    env->DeleteLocalRef(FocusScoreActivity);
    env->DeleteLocalRef(DetectEdgesActivity);
    env->DeleteLocalRef(Transform3DActivity);

    return JNI_VERSION_1_6;
}
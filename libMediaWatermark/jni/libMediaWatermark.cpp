#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <cstddef>
#include <stdint.h>
#include <string.h>

#include <GLES2/gl2.h>

//#define DEBUG_WATERMARK
#ifdef DEBUG_WATERMARK
#define LOG_TAG "Watermark"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG,__VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...) ;
#define LOGE(...) ;
#endif

static const char* kClassPathName = "com/media/watermark/WMSurfaceHolder";

static const char* VERTEX_SHADER =
"uniform mat4 uMVPMatrix;\n"
"uniform mat4 uSTMatrix;\n"
"attribute vec4 aPosition;\n"
"attribute vec4 aTextureCoord;\n"
"varying vec2 vTextureCoord;\n"
"void main() {\n"
"  gl_Position = uMVPMatrix * aPosition;\n"
"  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n"
"}\n";

static const char* FRAGMENT_SHADER =
"#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"varying vec2 vTextureCoord;\n"
"uniform samplerExternalOES sTexture;\n"
"void main() {\n"
"  gl_FragColor = texture2D(sTexture, vTextureCoord);\n"
"}\n";

static const char* FRAGMENT_SHADER_WATERMARK =
"#extension GL_OES_EGL_image_external : require\n"
"precision mediump float;\n"
"varying vec2 vTextureCoord;\n"
"uniform samplerExternalOES sTexture;\n"
"uniform sampler2D oTexture;\n"
"void main() {\n"
"  vec4 bg_color = texture2D(sTexture, vTextureCoord);\n"
"  vec4 fg_color = texture2D(oTexture, vTextureCoord);\n"
"  float colorR = (1.0 - fg_color.a) * bg_color.r + fg_color.a * fg_color.r;\n"
"  float colorG = (1.0 - fg_color.a) * bg_color.g + fg_color.a * fg_color.g;\n"
"  float colorB = (1.0 - fg_color.a) * bg_color.b + fg_color.a * fg_color.b;\n"
"  gl_FragColor = vec4(colorR, colorG, colorB, bg_color.a);\n"
"}\n";

static const uint32_t VERTEXES_SIZE = 16;
static const uint32_t TRIANGLE_VERTEXES_SIZE = 20;
static const float DEFAULT_TRIANGLE_VERTEXES_DATA[] =
{
    // X, Y, Z, U, V
    -1.0f, -1.0f, 0, 0.f, 0.f,
    1.0f, -1.0f, 0, 1.f, 0.f,
    -1.0f, 1.0f, 0, 0.f, 1.f,
    1.0f, 1.0f, 0, 1.f, 1.f,
};

static const float DEFAULT_TRANSFORMATION[] =
{
    1.0, 0.0, 0.0, 0.0,
    0.0, -1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 1.0, 0.0, 1.0
};

static JavaVM* sVm = NULL;
JNIEnv* getJNIEnv()
{
    JNIEnv* env = NULL;
    if (sVm->GetEnv((void**)&env, JNI_VERSION_1_2) != JNI_OK) {
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_2;
        args.name = NULL;
        args.group = NULL;
    }
    return env;
}

class WatermarkData {
public:
    WatermarkData()
        : mWidth(0)
        , mHeight(0)
        , mSize(0)
        , mData(0)
    {}

    WatermarkData(JNIEnv* env, jobject bitmap)
        : mWidth(0)
        , mHeight(0)
        , mSize(0)
        , mData(0)
    {
        AndroidBitmapInfo bitmapInfo;
        AndroidBitmap_getInfo(env, bitmap, &bitmapInfo);
        mWidth = bitmapInfo.width;
        mHeight = bitmapInfo.height;
        mSize = mWidth * mHeight * 4;

        void* bitmapPixels = NULL;
        AndroidBitmap_lockPixels(env, bitmap, &bitmapPixels);
        copy((uint8_t*)bitmapPixels, mSize);
        AndroidBitmap_unlockPixels(env, bitmap);
    }

    WatermarkData(const WatermarkData& other) { *this = other; }

    const WatermarkData& operator=(const WatermarkData& r)
    {
        mHeight = r.mHeight;
        mWidth = r.mWidth;
        mSize = r.mSize;
        copy(r.mData, r.mSize);
        return *this;
    }

    ~WatermarkData()
    {
        if (mData) {
            delete[] mData;
        }
    }

    void copy(uint8_t* data, uint32_t size)
    {
        if (mData)
            delete[] mData;

        if (size) {
            mData = new uint8_t[size];
            memcpy(mData, data, size);
        }
    }

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mSize;
    uint8_t* mData;
};

class Render {
public:
    Render();
    virtual ~Render();

    virtual void init();
    virtual const char* getFragmentShader() const { return FRAGMENT_SHADER; }
    virtual void draw(JNIEnv* env);

    int32_t getTextureID() const { return mTextureID; }
    void release();
    void getTransformMatrix(JNIEnv* env, jfloatArray array);

protected:
    void checkGlError(const char* op) const;
    void setIdentityMatrix(float sm[], int smOffset) const;
    void setEmptyMatrix(float sm[]) const;
    int32_t loadShader(int shaderType, const char* source) const;
    int32_t createProgram(const char* vertexSource, const char* fragmentSource) const;
    void drawStart();
    void drawSetupMatrix();
    void drawFinish();

    static const int32_t FLOAT_SIZE_BYTES = 4;
    static const int32_t TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
    static const int32_t TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
    static const int32_t TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;
    // Magic key GLES11Ext.GL_TEXTURE_EXTERNAL_OES needed for rendering MediaPlayer
    static const int32_t GL_TEXTURE_EXTERNAL_OES = 0x8D65;
    static const int32_t TARGET_TEXTURE_ID = GL_TEXTURE_EXTERNAL_OES;

    GLfloat mTriangleVerticesData[TRIANGLE_VERTEXES_SIZE];
    GLuint mTextures[1];

    GLfloat mMVPMatrix[16];
    GLfloat mSTMatrix[16];

    int32_t mProgram;
    int32_t mTextureID;
    int32_t muMVPMatrixHandle;
    int32_t muSTMatrixHandle;
    int32_t maPositionHandle;
    int32_t maTextureHandle;

    jfloatArray mTransformMatrix;
};

class WatermarkRender : public Render {
public:
    virtual void init();
    virtual const char* getFragmentShader() const { return FRAGMENT_SHADER_WATERMARK; }
    virtual void draw(JNIEnv* env);

    void drawWatermark(JNIEnv* env);
    void setData(const WatermarkData& data) { mData = data; }

    WatermarkData mData;
    GLuint mTexturesWatermark[1];
    int32_t moTextureHandle;
};

static Render* s_render = NULL;

Render::Render()
    : mProgram(0)
    , mTextureID(0)
    , muSTMatrixHandle(0)
    , muMVPMatrixHandle(0)
    , maPositionHandle(0)
    , maTextureHandle(0)
    , mTransformMatrix(0)
{
    memcpy(mTriangleVerticesData, DEFAULT_TRIANGLE_VERTEXES_DATA, sizeof(float) * TRIANGLE_VERTEXES_SIZE);
    memcpy(mSTMatrix, DEFAULT_TRANSFORMATION, sizeof(float) * VERTEXES_SIZE);

    setIdentityMatrix(mSTMatrix, 0);
    setEmptyMatrix(mMVPMatrix);

    glGenTextures(1, mTextures);
    mTextureID = mTextures[0];
}

Render::~Render()
{
    release();
}

void Render::init()
{
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTextureID);
    checkGlError("glBindTexture mTextureID " + GL_TEXTURE_EXTERNAL_OES);
    mProgram = createProgram(VERTEX_SHADER, getFragmentShader());
    if (mProgram == 0) {
        return;
    }
    maPositionHandle = glGetAttribLocation(mProgram, "aPosition");
    checkGlError("glGetAttribLocation aPosition");
    if (maPositionHandle == -1) {
        LOGE("Could not get attrib location for aPosition");
        return;
    }
    maTextureHandle = glGetAttribLocation(mProgram, "aTextureCoord");
    checkGlError("glGetAttribLocation aTextureCoord");
    if (maTextureHandle == -1) {
        LOGE("[Render] Could not get attrib location for aTextureCoord");
        return;
    }

    muMVPMatrixHandle = glGetUniformLocation(mProgram, "uMVPMatrix");
    checkGlError("glGetUniformLocation uMVPMatrix");
    if (muMVPMatrixHandle == -1) {
        LOGE("[Render] Could not get attrib location for uMVPMatrix");
        return;
    }

    muSTMatrixHandle = glGetUniformLocation(mProgram, "uSTMatrix");
    checkGlError("glGetUniformLocation uSTMatrix");
    if (muSTMatrixHandle == -1) {
        LOGE("[Render] Could not get attrib location for uSTMatrix");
        return;
    }

    // Can't do mipmapping with mediaplayer source
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    checkGlError("glTexParameteri mTextureID");

    LOGD("[Render] make success mTextureID=%d!", mTextureID);
}

void Render::drawStart()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(TARGET_TEXTURE_ID, mTextureID);
    checkGlError("glBindTexture");

    float* data = &mTriangleVerticesData[0];
    data += TRIANGLE_VERTICES_DATA_POS_OFFSET;
    glVertexAttribPointer(maPositionHandle, 3, GL_FLOAT,
            false, TRIANGLE_VERTICES_DATA_STRIDE_BYTES,
            mTriangleVerticesData);
    checkGlError("glVertexAttribPointer maPosition");
    glEnableVertexAttribArray(maPositionHandle);
    checkGlError("glEnableVertexAttribArray maPositionHandle");

    data = &mTriangleVerticesData[0];
    data += TRIANGLE_VERTICES_DATA_UV_OFFSET;

    glVertexAttribPointer(maTextureHandle, 3, GL_FLOAT,
            false, TRIANGLE_VERTICES_DATA_STRIDE_BYTES,
            data);
    checkGlError("glVertexAttribPointer maTextureHandle");

    glEnableVertexAttribArray(maTextureHandle);
    checkGlError("glEnableVertexAttribArray maTextureHandle");
}

void Render::drawSetupMatrix()
{
    memcpy(mSTMatrix, DEFAULT_TRANSFORMATION, sizeof(float) * VERTEXES_SIZE);
    setIdentityMatrix(mMVPMatrix, 0);
    glUniformMatrix4fv(muMVPMatrixHandle, 1, GL_FALSE, mMVPMatrix);
    glUniformMatrix4fv(muSTMatrixHandle, 1, GL_FALSE, mSTMatrix);
    checkGlError("glUniformMatrix4fv");
}

void Render::drawFinish()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    checkGlError("glDrawArrays");
}

void Render::getTransformMatrix(JNIEnv* env, jfloatArray array)
{
    if (float* ptr = env->GetFloatArrayElements(array, NULL)) {
        for (uint32_t i = 0; i < VERTEXES_SIZE; ++i) {
            ptr[i] = mSTMatrix[i];
        }
        env->ReleaseFloatArrayElements(array, ptr, 0);
    }
}

void Render::release()
{
    glDeleteTextures(1, mTextures);
    checkGlError("glDeleteTextures");
}

void Render::draw(JNIEnv* env)
{
    drawStart();
    drawSetupMatrix();
    drawFinish();
}

void Render::checkGlError(const char* op) const
{
    int error = glGetError();
    if (error != GL_NO_ERROR) {
        LOGE("[Render] error %s : glError %d", op, error);
    }
}

void Render::setIdentityMatrix(float* sm, int smOffset) const
{
    for (int32_t i = 0 ; i < VERTEXES_SIZE ; i++) {
        sm[smOffset + i] = 0;
    }
    for(int32_t i = 0; i < VERTEXES_SIZE; i += 5) {
        sm[smOffset + i] = 1.0f;
    }
}

void Render::setEmptyMatrix(float* sm) const
{
    for (int32_t i = 0 ; i < VERTEXES_SIZE ; i++) {
        sm[i] = 0;
    }
}

int32_t Render::loadShader(int shaderType, const char* source) const
{
    GLuint shader = glCreateShader(shaderType);
    if (shader != 0) {
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            LOGE("[Render] Could not compile shader type=%d", shaderType);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

int32_t Render::createProgram(const char* vertexSource, const char* fragmentSource) const
{
    int vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) {
        return 0;
    }
    int pixelShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (pixelShader == 0) {
        return 0;
    }

    int32_t program = glCreateProgram();
    if (program != 0) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked) {
            LOGE("[Render] Could not link program: %d", program);
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

void WatermarkRender::init()
{
    Render::init();

    moTextureHandle = glGetUniformLocation(mProgram, "oTexture");
    checkGlError("glGetAttribLocation oTexture");
    if (maTextureHandle == -1) {
        return;
    }

    glGenTextures(1, mTexturesWatermark);
    glBindTexture(GL_TEXTURE_2D, mTexturesWatermark[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    LOGD("[WatermarkRender] make success mTextureID=%d!", mTextureID);
}

void WatermarkRender::draw(JNIEnv* env)
{
    drawStart();
    drawSetupMatrix();
    drawWatermark(env);
    drawFinish();
}

void WatermarkRender::drawWatermark(JNIEnv* env)
{
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTexturesWatermark[0]);
    checkGlError("glBindTexture");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mData.mWidth, mData.mHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, mData.mData);

    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mData.mWidth, mData.mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mData.mData);
    checkGlError("texImage2d");
    glUniform1i(moTextureHandle, 1);
    checkGlError("oTextureHandle - glUniform1i");
}

static void media_watermark_init(JNIEnv* env, jobject thiz, jobject jwatermark)
{
    if (!s_render) {
        WatermarkRender* render = new WatermarkRender();

        WatermarkData data(env, jwatermark);
        render->setData(data);
        s_render = render;
        s_render->init();

    }
}

static int32_t media_watermark_get_id(JNIEnv* env, jobject thiz)
{
    return s_render ? s_render->getTextureID() : -1;
}

static void media_watermark_release(JNIEnv* env, jobject thiz)
{
    if (s_render) {
        delete s_render;
        s_render = NULL;
    }
}

static void media_watermark_draw(JNIEnv* env, jobject thiz, jfloatArray matrix, jint width, jint height)
{
    if (s_render) {
        s_render->draw(env);
        s_render->getTransformMatrix(env, matrix);
    }
}

static JNINativeMethod gMethods[] = {
    { "native_init", "(Landroid/graphics/Bitmap;)V",  (void*) media_watermark_init },
    { "native_get_textureId", "()I",                  (void*) media_watermark_get_id },
    { "native_release", "()V",                        (void*) media_watermark_release },
    { "native_draw", "([FII)V",                       (void*) media_watermark_draw },
};

#ifndef NELEM
# define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    sVm = vm;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK)
        return result;

    jclass clazz = env->FindClass(kClassPathName);
    if (clazz == NULL || env->RegisterNatives(clazz, gMethods, NELEM(gMethods)) < 0)
        return result;

    return JNI_VERSION_1_6;
}

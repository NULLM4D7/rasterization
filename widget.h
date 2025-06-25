#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QOpenGLExtraFunctions>
#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>

class Widget : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
    Q_OBJECT
    QOpenGLShaderProgram mainShaderP;       // 主着色器参数
    QOpenGLShaderProgram shadowShaderP;     // 阴影着色器参数
    QOpenGLVertexArrayObject m_vao;         // 顶点数组对象
    QOpenGLBuffer m_vbo;                    // 顶点缓冲区对象
    QOpenGLBuffer m_ebo;                    // 索引缓冲区对象
    QOpenGLFramebufferObject* shadowFBO;    // 阴影纹理缓冲帧对象
    QVector<float> vertexVec;               // 顶点容器
    QVector<unsigned int> indexs;           // 顶点索引容器
    QMatrix4x4 projection;
    QMatrix3x3 cameraRot;                   // 摄像机旋转
    QVector3D cameraPos;                    // 单独存储摄像机坐标 避免精度误差
    QVector3D lightPos;
    QMatrix4x4 lightSpaceMatrix;            // 光源空间变换矩阵

    QPoint* pressPoint;                     // 记录鼠标按下时坐标，长按时实时更新

    QMatrix4x4 getCamera() const;

    // 顶点
    struct Vertex
    {
        QVector3D* coord;
        QVector3D* norm;
        QVector2D* uv;
    };
#define VERTEX_SIZE 8   // 一个顶点使用float变量数
    // 物体
    struct Obj
    {
        QVector<float> vertexs;             // 顶点属性容器
        QVector<unsigned int> indexs;       // 顶点索引容器
        void transform(QMatrix4x4 model);   // 线性变换物体
    };
    Obj getCube(QMatrix4x4 model) const;
    Obj getPlane(QMatrix4x4 model) const;
    void addObj(const Obj& obj);

    QOpenGLTexture *depthTexture;   // 光源视角下深度纹理
    void updateDepthTexture();      // 更新光源视角下深度纹理

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mousePressEvent(QMouseEvent* me) override;
    void mouseReleaseEvent(QMouseEvent* me) override;
    void mouseMoveEvent(QMouseEvent* me) override;
    void keyPressEvent(QKeyEvent* ke) override;
public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
};

#endif // WIDGET_H

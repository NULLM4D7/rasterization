#include "widget.h"
#include <QPainter>
#include <QMouseEvent>
#include "shader.h"

Widget::Widget(QWidget *parent)
    : QOpenGLWidget(parent),
    m_vbo{QOpenGLBuffer::VertexBuffer},
    m_ebo{QOpenGLBuffer::IndexBuffer}
{
    setWindowTitle("Rasterization");
    pressPoint = nullptr;
    shadowFBO = nullptr;
    depthTexture = nullptr;
}

Widget::~Widget()
{
    if (pressPoint)
    {
        delete pressPoint;
        pressPoint = nullptr;
    }
    if (shadowFBO)
    {
        delete shadowFBO;
        shadowFBO = nullptr;
    }
    if (depthTexture)
    {
        delete depthTexture;
        depthTexture = nullptr;
    }
}

// 获取摄像机变换矩阵
QMatrix4x4 Widget::getCamera() const
{
    QMatrix4x4 camera(cameraRot);
    camera.setColumn(3, QVector4D(cameraPos, 1));
    return camera;
}

// 对物体线性变换
void Widget::Obj::transform(QMatrix4x4 model)
{
    for (int i = 0; i < vertexs.size(); i += VERTEX_SIZE)
    {
        QVector4D coord, norm;
        for (int j = 0; j < 3; j++)
        {
            coord[j] = vertexs[i + j];
            norm[j] = vertexs[i + j + 3];
        }
        coord[3] = 1.0f;
        norm[3] = 0.0f;
        coord = model * coord;
        norm = model.inverted().transposed() * norm;
        for (int j = 0; j < 3; j++)
        {
            vertexs[i + j] = coord[j];
            vertexs[i + j + 3] = norm[j];
        }
    }
}

// 获取立方体对象
Widget::Obj Widget::getCube(QMatrix4x4 model = QMatrix4x4()) const
{
    // 顶点坐标
    QVector<QVector3D> coords = {
        {0.5f, 0.5f, 0.5f},     // 0
        {-0.5f, 0.5f, 0.5f},    // 1
        {-0.5f, -0.5f, 0.5f},   // 2
        {0.5f, -0.5f, 0.5f},    // 3
        {0.5f, 0.5f, -0.5f},    // 4
        {-0.5f, 0.5f, -0.5f},   // 5
        {-0.5f, -0.5f, -0.5f},  // 6
        {0.5f, -0.5f, -0.5f}    // 7
    };
    // 顶点法线
    QVector<QVector3D> norms;
    for (const QVector3D& coord : coords)
    {
        norms.push_back(coord / 0.5f);
    }
    // 纹理坐标
    QVector<QVector2D> uvs = {
        QVector2D(0.0f, 0.0f),
        QVector2D(1.0f, 0.0f),
        QVector2D(1.0f, 1.0f),
        QVector2D(0.0f, 1.0f)
    };
    QVector<Vertex> vertexs;
    vertexs.push_back(Vertex{&coords[5], &norms[5], &uvs[0]});    // 0
    vertexs.push_back(Vertex{&coords[1], &norms[1], &uvs[1]});    // 1
    vertexs.push_back(Vertex{&coords[2], &norms[2], &uvs[2]});    // 2
    vertexs.push_back(Vertex{&coords[6], &norms[6], &uvs[3]});    // 3

    vertexs.push_back(Vertex{&coords[1], &norms[1], &uvs[0]});    // 4
    vertexs.push_back(Vertex{&coords[0], &norms[0], &uvs[1]});    // 5
    vertexs.push_back(Vertex{&coords[3], &norms[3], &uvs[2]});    // 6
    vertexs.push_back(Vertex{&coords[2], &norms[2], &uvs[3]});    // 7

    vertexs.push_back(Vertex{&coords[0], &norms[0], &uvs[0]});    // 8
    vertexs.push_back(Vertex{&coords[4], &norms[4], &uvs[1]});    // 9
    vertexs.push_back(Vertex{&coords[7], &norms[7], &uvs[2]});    // 10
    vertexs.push_back(Vertex{&coords[3], &norms[3], &uvs[3]});    // 11

    vertexs.push_back(Vertex{&coords[4], &norms[4], &uvs[0]});    // 12
    vertexs.push_back(Vertex{&coords[5], &norms[5], &uvs[1]});    // 13
    vertexs.push_back(Vertex{&coords[6], &norms[6], &uvs[2]});    // 14
    vertexs.push_back(Vertex{&coords[7], &norms[7], &uvs[3]});    // 15

    vertexs.push_back(Vertex{&coords[5], &norms[5], &uvs[0]});    // 16
    vertexs.push_back(Vertex{&coords[4], &norms[4], &uvs[1]});    // 17
    vertexs.push_back(Vertex{&coords[0], &norms[0], &uvs[2]});    // 18
    vertexs.push_back(Vertex{&coords[1], &norms[1], &uvs[3]});    // 19

    vertexs.push_back(Vertex{&coords[2], &norms[2], &uvs[0]});    // 20
    vertexs.push_back(Vertex{&coords[3], &norms[3], &uvs[1]});    // 21
    vertexs.push_back(Vertex{&coords[7], &norms[7], &uvs[2]});    // 22
    vertexs.push_back(Vertex{&coords[6], &norms[6], &uvs[3]});    // 23

    Obj cube;

    // 设置立方体顶点坐标、法线和纹理坐标
    for (const Vertex& vertex : vertexs)
    {
        cube.vertexs.push_back(vertex.coord->x());
        cube.vertexs.push_back(vertex.coord->y());
        cube.vertexs.push_back(vertex.coord->z());

        cube.vertexs.push_back(vertex.norm->x());
        cube.vertexs.push_back(vertex.norm->y());
        cube.vertexs.push_back(vertex.norm->z());

        cube.vertexs.push_back(vertex.uv->x());
        cube.vertexs.push_back(vertex.uv->y());
    }

    // 设置立方体三角面顶点索引
    for (unsigned int i = 0; i < 6; i++)
    {
        // 第i个面
        unsigned int startI = i * 4;    // 每个立方体面耗用4个顶点
        // 一个立方体面由两个三角面组成
        cube.indexs.push_back(startI);
        cube.indexs.push_back(startI + 2);
        cube.indexs.push_back(startI + 1);

        cube.indexs.push_back(startI);
        cube.indexs.push_back(startI + 3);
        cube.indexs.push_back(startI + 2);
    }
    cube.transform(model);
    return cube;
}

// 获取平面对象
Widget::Obj Widget::getPlane(QMatrix4x4 model) const
{
    // 顶点坐标
    QVector<QVector3D> coords = {
        QVector3D{0.5f, 0.0f, 0.5f},
        QVector3D{-0.5f, 0.0f, 0.5f},
        QVector3D{-0.5f, 0.0f, -0.5f},
        QVector3D{0.5f, 0.0f, -0.5f}
    };
    // 顶点法线
    QVector<QVector3D> norms;
    for (int i = 0; i < 4; i++)
    {
        norms.push_back(QVector3D(0.0f, 1.0f, 0.0f));
    }
    // 纹理坐标
    QVector<QVector2D> uvs = {
        QVector2D(1.0f, 0.0f),
        QVector2D(0.0f, 0.0f),
        QVector2D(0.0f, 1.0f),
        QVector2D(1.0f, 1.0f)
    };
    QVector<Vertex> vertexs;
    for (int i = 0; i < 4; i++)
    {
        vertexs.push_back(Vertex{&coords[i], &norms[i], &uvs[i]});
    }
    Obj plane;
    for (int i = 0; i < 4; i++)
    {
        plane.vertexs.push_back(vertexs[i].coord->x());
        plane.vertexs.push_back(vertexs[i].coord->y());
        plane.vertexs.push_back(vertexs[i].coord->z());

        plane.vertexs.push_back(vertexs[i].norm->x());
        plane.vertexs.push_back(vertexs[i].norm->y());
        plane.vertexs.push_back(vertexs[i].norm->z());

        plane.vertexs.push_back(vertexs[i].uv->x());
        plane.vertexs.push_back(vertexs[i].uv->y());
    }
    plane.indexs.push_back(0);
    plane.indexs.push_back(1);
    plane.indexs.push_back(3);

    plane.indexs.push_back(2);
    plane.indexs.push_back(3);
    plane.indexs.push_back(1);

    plane.transform(model);
    return plane;
}

// 向场景中添加物体对象
void Widget::addObj(const Widget::Obj& obj)
{
    const int startI = vertexVec.size() / VERTEX_SIZE;  // 该物体顶点存放位置的起始索引
    for(float vertex : obj.vertexs) // 将顶点属性写入
    {
        vertexVec.push_back(vertex);
    }
    for (unsigned int index : obj.indexs)   // 将索引写入
    {
        indexs.push_back(index + startI);
    }
}

// 更新光源视角下深度纹理
void Widget::updateDepthTexture()
{
    // 渲染光源坐标系下深度纹理
    // 更新缓冲帧
    if (shadowFBO) delete shadowFBO;
    shadowFBO = new QOpenGLFramebufferObject(width(), height(), QOpenGLFramebufferObject::Depth);
    shadowFBO->bind();
    // 更新深度纹理
    if (depthTexture) delete depthTexture;
    depthTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    depthTexture->setSize(width(), height());
    depthTexture->setFormat(QOpenGLTexture::D32);
    depthTexture->allocateStorage();
    // 将深度纹理附加到缓冲帧对象上
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTexture->textureId(), 0);

    glClear(GL_DEPTH_BUFFER_BIT); // 清除深度缓冲
    m_vao.bind();   // 激活顶点缓冲对象
    shadowShaderP.bind();
    shadowShaderP.setUniformValue("lightSpaceMatrix", lightSpaceMatrix);
    // 写入深度
    glDrawElements(GL_TRIANGLES, indexs.size(), GL_UNSIGNED_INT, 0);

    shadowFBO->release();
    depthTexture->release();
    shadowShaderP.release();
    m_vao.release();
}

void Widget::mousePressEvent(QMouseEvent* me)
{
    pressPoint = new QPoint(me->pos());
    return QWidget::mousePressEvent(me);
}
void Widget::mouseReleaseEvent(QMouseEvent* me)
{
    if (pressPoint)
    {
        delete pressPoint;
        pressPoint = nullptr;
    }
    return QWidget::mouseReleaseEvent(me);
}
void Widget::mouseMoveEvent(QMouseEvent* me)
{
    if (pressPoint)
    {
        QPoint delta = me->pos() - *pressPoint; // 上次回调时坐标指向本次回调时坐标的向量

        QVector4D v(1.0f, 0.0f, 0.0f, 0.0f);   // 摄像机向右向量
        QMatrix4x4 cameraO(cameraRot);  // 回到原点的摄像机变换矩阵
        v = cameraO * v;

        QMatrix4x4 pitchM, rowM;
        pitchM.rotate(-delta.y() / 5.0, v.toVector3D());    // 俯仰角
        rowM.rotate(-delta.x() / 5.0, QVector3D(0, 1, 0));  // 偏航角
        cameraO = rowM * pitchM * cameraO;

        cameraRot = cameraO.normalMatrix();

        update();
        pressPoint = new QPoint(me->pos());
    }
    return QWidget::mouseMoveEvent(me);
}

void Widget::keyPressEvent(QKeyEvent* ke)
{
    switch(ke->key())
    {
    case Qt::Key_W:
    {
        QVector4D v(0, 0, -1, 0);   // 向前向量
        v = QMatrix4x4(cameraRot) * v;
        v *= 0.1;
        cameraPos += v.toVector3D();
        break;
    }
    case Qt::Key_A:
    {
        QVector4D v(-1, 0, 0, 0);   // 向左向量
        v = QMatrix4x4(cameraRot) * v;
        v *= 0.1;
        cameraPos += v.toVector3D();
        break;
    }
    case Qt::Key_D:
    {
        QVector4D v(1, 0, 0, 0);   // 向右向量
        v = QMatrix4x4(cameraRot) * v;
        v *= 0.1;
        cameraPos += v.toVector3D();
        break;
    }
    case Qt::Key_S:
    {
        QVector4D v(0, 0, 1, 0);   // 向后向量
        v = QMatrix4x4(cameraRot) * v;
        v *= 0.1;
        cameraPos += v.toVector3D();
        break;
    }
    case Qt::Key_Q:
    {
        cameraPos += QVector3D(0, -0.1, 0);
        break;
    }
    case Qt::Key_E:
    {
        cameraPos += QVector3D(0, 0.1, 0);
        break;
    }
    default:
        break;
    }
    update();
}

void Widget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // 创建立方体
    QMatrix4x4 mat1;
    mat1.translate(-0.75f, 0.0f, 0.0f);
    addObj(getCube(mat1));
    QMatrix4x4 mat2;
    mat2.translate(0.75f, 0.0f, 0.0f);
    addObj(getCube(mat2));
    // 创建平面
    QMatrix4x4 mat3;
    mat3.scale(5);
    QMatrix4x4 mat3_1;
    mat3_1.translate(QVector3D(0.0f, -0.5f, 0.0f));
    addObj(getPlane(mat3_1 * mat3));

    int coordSize = sizeof(*Vertex::coord);         // 坐标占用字节数
    int normSize = sizeof(*Vertex::norm);           // 法线占用字节数
    int vertexSize = VERTEX_SIZE * sizeof(float);   // 每个顶点占用字节数

    // 缓冲区
    m_vao.create();
    m_vbo.create();
    m_ebo.create();

    shadowShaderP.create();
    mainShaderP.create();

    // 编译并链接光源视角下深度缓冲着色器
    shadowShaderP.addShaderFromSourceCode(QOpenGLShader::Vertex, shadowVertexShader);
    shadowShaderP.addShaderFromSourceCode(QOpenGLShader::Fragment, shadowFragShader);
    shadowShaderP.link();

    // 编译并链接主着色器
    mainShaderP.addShaderFromSourceCode(QOpenGLShader::Vertex, mainVertShader);
    mainShaderP.addShaderFromSourceCode(QOpenGLShader::Fragment, mainFragShader);
    mainShaderP.link();

    m_vao.bind();   // 记录顶点属性配置
    m_vbo.bind();
    m_ebo.bind();

    // 将数据分配到缓冲区
    m_vbo.allocate(vertexVec.data(), sizeof(float) * vertexVec.size());
    m_ebo.allocate(indexs.data(), sizeof(unsigned int) * indexs.size());

    // 位置属性 (location=0) 设置顶点坐标属性在缓冲区中的位置
    mainShaderP.enableAttributeArray(0);
    mainShaderP.setAttributeBuffer(0, GL_FLOAT, 0, 3, vertexSize);
    shadowShaderP.enableAttributeArray(0);
    shadowShaderP.setAttributeBuffer(0, GL_FLOAT, 0, 3, vertexSize);

    // 法线属性 (location=1) 设置顶点法线属性在缓冲区中的位置
    mainShaderP.enableAttributeArray(1);
    mainShaderP.setAttributeBuffer(1, GL_FLOAT, coordSize, 3, vertexSize);

    // 纹理坐标属性 (location=2) 设置顶点纹理坐标属性在缓冲区中的位置
    mainShaderP.enableAttributeArray(2);
    mainShaderP.setAttributeBuffer(2, GL_FLOAT, coordSize + normSize, 2, vertexSize);

    m_vao.release();

    cameraPos = QVector3D(0, 0, 3); // 摄像机坐标 摄像机在原点，朝向-Z，向上向量朝向Y
    projection.perspective(60.0f, (float)width()/(float)height(), 0.1f, 100.0f);    // 垂直视角范围为60度
    lightPos = QVector3D(-15.0f, 5.0f, 10.0f);  // 光源坐标

    // 光源空间变化矩阵
    QMatrix4x4 lightProj;
    lightProj.ortho(-5.0f, 5.0f, -5.0f, 5.0f, 1.0f, 50.0f);
    QMatrix4x4 lightView;
    lightView.lookAt(lightPos, QVector3D(0, 0, 0), QVector3D(0,1,0));
    lightSpaceMatrix = lightProj * lightView;

    glEnable(GL_DEPTH_TEST); // 启用深度检测
}

void Widget::paintGL()
{
    m_vao.bind();   // 激活顶点数据缓冲

    // 渲染主场景
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 清除颜色缓存和深度缓存
    mainShaderP.bind();
    // 顶点着色器参数
    mainShaderP.setUniformValue("cameraSpaceMat", projection * getCamera().inverted());
    mainShaderP.setUniformValue("lightSpaceMatrix", lightSpaceMatrix);

    // 片段着色器参数
    mainShaderP.setUniformValue("lightPos", lightPos);    // 光源位置
    mainShaderP.setUniformValue("cameraPos", cameraPos);  // 相机位置
    mainShaderP.setUniformValue("diff", QVector3D(0.8f, 0.8f, 0.8f)); // 漫反射率
    mainShaderP.setUniformValue("i", 500.0f);  // 与光源距离为1时能量

    // 绑定纹理
    QOpenGLTexture texture(QImage("C:\\Users\\86159\\Desktop\\pic\\1687922726788889.png"));
    texture.bind(0); // 绑定到纹理单元0
    mainShaderP.setUniformValue("pic", 0); // 传递纹理单元索引
    // 绑定深度纹理
    depthTexture->bind(1);
    mainShaderP.setUniformValue("shadowMap", 1);

    // 绘制
    glDrawElements(GL_TRIANGLES, indexs.size(), GL_UNSIGNED_INT, 0);

    mainShaderP.release();
    m_vao.release();
}

void Widget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);  // 调整视口
    projection.setToIdentity(); // 设置为单位矩阵
    projection.perspective(60.0f, (float)w/(float)h, 0.1f, 100.0f);  // 垂直视角范围为60度

    updateDepthTexture();
}

#ifndef SHADER_H
#define SHADER_H

const char *mainVertShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoord;
    uniform mat4 cameraSpaceMat;    // 透视变换矩阵*摄像机逆变换矩阵
    uniform mat4 lightSpaceMatrix;  // 光源视角投影*光源逆变换矩阵
    // 传递给片段着色器的参数
    out vec3 FragPos;               // 片段坐标
    out vec3 Normal;                // 法线
    out vec2 TexCoord;              // 纹理坐标
    out vec4 FragPosLightSpace;     // 片段在光源空间的坐标
    void main() {
        FragPos = aPos;
        Normal = aNormal;
        TexCoord = aTexCoord;
        FragPosLightSpace = lightSpaceMatrix * vec4(aPos, 1.0);
        gl_Position = cameraSpaceMat * vec4(aPos, 1.0);   // 摄像机坐标系下的顶点坐标
    }
)";

const char *mainFragShader = R"(
    #version 330 core
    in vec3 FragPos;            // 世界坐标系下片段坐标
    in vec3 Normal;             // 世界坐标系下片段法线
    in vec2 TexCoord;           // 片段纹理坐标
    in vec4 FragPosLightSpace;  // 片段在光源空间的坐标
    out vec4 FragColor;
    uniform vec3 lightPos;      // 世界坐标系下光源位置
    uniform vec3 diff;          // 漫反射率
    uniform float i;            // 与光源距离为1时能量
    uniform vec3 cameraPos;     // 摄像机位置
    uniform sampler2D pic;      // 贴图
    uniform sampler2DShadow shadowMap;    // 深度纹理
    void main() {
        vec4 baseColor = texture(pic, TexCoord); // 采样纹理
        // 漫反射
        vec3 norm = normalize(Normal);
        vec3 l = lightPos - FragPos;    // 片段坐标指向光源坐标
        vec3 lightDir = normalize(l);   // 归一化
        float cosVal = max(dot(norm, lightDir), 0.0);
        float i_frag = i / pow(length(l), 2);  // 片段所在坐标接收到的能量
        vec3 diffuse = i_frag * diff * cosVal;
        // 镜面反射
        vec3 v = normalize(cameraPos - FragPos);   // 片段坐标指向相机坐标
        cosVal = max(pow(dot(norm, normalize(v + lightDir)), 200), 0.0);
        vec3 specular = vec3(0.5, 0.5, 0.5) * i_frag * cosVal;
        // 环境光照
        vec3 ambient = diff * 0.4;

        // 阴影
        vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        // 比较光源坐标系下片段深度与shadowMap中对应坐标的深度
        float shadow = texture(shadowMap, vec3(projCoords.xy, projCoords.z * 0.996));

        FragColor = vec4(ambient + (diffuse + specular) * shadow, 1.0) * baseColor;
    }
)";

const char *shadowVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 lightSpaceMatrix;  // 光源视角投影*光源逆变换矩阵
    void main() {
        gl_Position = lightSpaceMatrix * vec4(aPos, 1.0);
    }
)";

const char *shadowFragShader = R"(
    #version 330 core
    void main() {
    }
)";

#endif // SHADER_H

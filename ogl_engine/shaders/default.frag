#version 330 core
out vec4 FragColor;
in vec4 color;
in vec2 uv;

uniform sampler2D tex;

void main()
{
   //FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
   //FragColor = color;
   FragColor = texture(tex, uv);
}
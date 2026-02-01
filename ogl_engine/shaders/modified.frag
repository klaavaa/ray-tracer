#version 330 core
out vec4 FragColor;
in vec4 color;
in vec2 uv;

uniform sampler2D tex;

void main()
{
	// inverted
	//FragColor = vec4(vec3(1.0 - texture(tex, uv)), 1.0);
	FragColor = texture(tex, uv);
    float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b; // balanced greyscale
    FragColor = vec4(average, average, average, 1.0);
}
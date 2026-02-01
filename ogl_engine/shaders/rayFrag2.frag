#version 460 core
out vec4 FragColor;
in vec4 color;
in vec2 uv;

uniform sampler2D tex;
uniform vec2 tex_size;

uniform sampler2D old_texture;
uniform sampler2D new_texture;

uniform int rendered_frames_count;
uniform int fraction_pixel_per_frame;
uniform uint time;
void main() {

    vec4 old_color = texture(old_texture, uv);
    vec4 new_color = texture(new_texture, uv);

    if (((uint(gl_FragCoord.y) + time) % fraction_pixel_per_frame) != 0)
    {
        FragColor = old_color;
        return;
    }

    float frame_counter = float((rendered_frames_count-1) / fraction_pixel_per_frame+1);

    float weight = 1.f / float(frame_counter);

    FragColor = old_color * (1 - weight) + new_color * weight;
}


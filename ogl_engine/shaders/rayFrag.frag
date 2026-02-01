#version 460 core
out vec4 frag_color;
in vec4 gl_FragCoord;

//layout(location = 0) out vec4 frag_color;



uniform uint time;
uniform ivec2 resolution;

#define MAX_VERTEX_COUNT 5000
#define MAX_TRIMESH_COUNT 5
#define MAX_INDICES_COUNT 5000
#define MAX_SPHERE_COUNT 100



struct Material
{
    vec3 color;
    vec3 emission_color;
    float emission_strenght;
    float reflection_multiplier;
};

struct AxisAllignedBox
{
    float p1[3];
    float p2[3];
};

struct TriMesh
{
    float vertices[MAX_VERTEX_COUNT][3];
    int indices[MAX_INDICES_COUNT][3];
    
    float color[3];
    float emission[4];
    float reflection;
 

    AxisAllignedBox box;
    
};

struct _Sphere
{
    float center[3];
    float radius;

    float color[3];
    float emission[4];
    float reflection;

};

layout(std430, binding = 0) buffer triMeshBuffer 
{
    TriMesh trimesh_array[MAX_TRIMESH_COUNT];
};

layout(std430, binding = 1) buffer sphereBuffer
{
    _Sphere sphere_array[MAX_SPHERE_COUNT];
};



uniform vec3 camera;
uniform vec2 camera_rotation;
uniform float focal_length;
uniform vec3 sky_color;
uniform vec3 horizont_color;

uniform int samples_per_pixel;
uniform int bounces;
uniform int fraction_pixel_per_frame;

uniform int triangle_count[MAX_TRIMESH_COUNT];
uniform int trimesh_count;
uniform int sphere_count;


float random(inout uint seed)
{
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;

    return float(seed) / 4294967295.0;
}

float random_normal_distribution(inout uint seed)
{
    float theta = 2 * 3.1415926 * random(seed);
    float rho = sqrt(-2 * log(random(seed)));
    return rho * cos(theta);
}

vec3 random_dir(inout uint seed)
{

  
    float x = random_normal_distribution(seed);
    float y = random_normal_distribution(seed);
    float z = random_normal_distribution(seed);


    return normalize(vec3(x, y, z));

}

vec3 random_hemisphere_dir(vec3 normal, inout uint seed)
{
    vec3 dir = random_dir(seed);
    return dir * sign(dot(normal, dir));
}

struct Ray
{
    vec3 origin;
    vec3 dir;

};



struct HitInfo
{
    vec3 p;
    vec3 normal;
    float t;

    bool front_face;

    Material material;

};

struct Sphere
{
    vec3 center;
    float radius;
    Material material;

};

struct Triangle
{
    vec3 a;
    vec3 b;
    vec3 c;
    Material material;
};


vec3 at(Ray r, float t)
{
    return r.origin + t * r.dir;
}

bool hit_sphere(Sphere sphere, Ray r, float t_min, float t_max, inout HitInfo hit_info)
{
    vec3 oc = r.origin - sphere.center;
    float dir_len = length(r.dir);
    float a = dir_len * dir_len;
    float half_b = dot(oc, r.dir);
    
    float oc_len = length(oc);
    float c = oc_len * oc_len - sphere.radius * sphere.radius;

    float discriminant = half_b  * half_b - a * c;
    if (discriminant < 0.f)
    {
        return false;
    }
    float sqrtd = sqrt(discriminant);

    float root = (-half_b - sqrtd) / a;

    if (root < t_min || t_max < root)
    {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root)
        {
            return false;
        }
    }

    hit_info.t = root;
    hit_info.p = at(r, hit_info.t);
    hit_info.normal = (hit_info.p - sphere.center) / sphere.radius;
    hit_info.material = sphere.material;
    if (dot(r.dir, hit_info.normal) > 0.f)
    {
        hit_info.normal *= -1.f;
        hit_info.front_face = false;
    }
    else
    {
        hit_info.front_face = true;
    }
    return true;
}

bool hit_triangle(Triangle triangle, Ray r, float t_min, float t_max, inout HitInfo hit_info)
{
    const float epsilon = 0.001;

    vec3 edge1 = triangle.b - triangle.a;
    vec3 edge2 = triangle.c - triangle.a;
    vec3 ray_cross_e2 = cross(r.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

  
    

    if (det > -epsilon && det < epsilon)
        return false;



    float inv_det = 1.0 / det;
    vec3 s = r.origin - triangle.a;
    float u = inv_det * dot(s, ray_cross_e2);

    if (u < 0 || u > 1)
        return false;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(r.dir, s_cross_e1);

    if (v < 0 || u + v > 1)
        return false;

    float t = inv_det * dot(edge2, s_cross_e1);
    
    if (t < t_min || t_max < t)
        return false;
        
    

    if (t > epsilon)
    {
       
       

        hit_info.t = t;
        hit_info.material = triangle.material;
        hit_info.p = at(r, hit_info.t);
        hit_info.normal = normalize(cross(edge1, edge2));
        if (dot(r.dir, hit_info.normal) > 0.f)
        {
            hit_info.normal *= -1.f;
            hit_info.front_face = false;
        }
        else
        {
            hit_info.front_face = true;
        }

        return true;

    }

    return false;
}

bool hit_axis_alligned_box(AxisAllignedBox box, Ray r)
{
    vec3 p1 = vec3(box.p1[0], box.p1[1], box.p1[2]) - r.origin;
    vec3 p2 = vec3(box.p2[0], box.p2[1], box.p2[2]) - r.origin;

    float near = -1.0 / .0;
    float far = 1.0 / .0;

    float t1 = p1.x / r.dir.x;
    float t2 = p2.x / r.dir.x;

    float tmin = min(t1, t2);
    float tmax = max(t1, t2);

    if (tmin > near) near = tmin;
    if (tmax < far) far = tmax;

    if (near > far || far < 0)
    {
        return false;
    }

    t1 = p1.y / r.dir.y;
    t2 = p2.y / r.dir.y;

    tmin = min(t1, t2);
    tmax = max(t1, t2);

    if (tmin > near) near = tmin;
    if (tmax < far) far = tmax;

    if (near > far || far < 0)
    {
        return false;
    }

    t1 = p1.z / r.dir.z;
    t2 = p2.z / r.dir.z;

    tmin = min(t1, t2);
    tmax = max(t1, t2);

    if (tmin > near) near = tmin;
    if (tmax < far) far = tmax;

    if (near > far || far < 0)
    {
        return false;
    }

    return true;

}

bool cast_ray(Ray r, inout HitInfo hit_info)
{

    float closest = 1.f / 0.f;
    bool hit = false;
    
    for (int i = 0; i < sphere_count; i++)
    {
        vec3 center = vec3(sphere_array[i].center[0], sphere_array[i].center[1], sphere_array[i].center[2]);
        float radius = sphere_array[i].radius;
        vec3 color = vec3(sphere_array[i].color[0],sphere_array[i].color[1], sphere_array[i].color[2]);
        vec4 emission = vec4(sphere_array[i].emission[0], sphere_array[i].emission[1], sphere_array[i].emission[2], sphere_array[i].emission[3]);
        float reflection = sphere_array[i].reflection;
        Sphere sphere = Sphere(center, radius, Material(color, emission.rgb, emission.w, reflection));
        if (hit_sphere(sphere, r, 0, closest, hit_info))
        {
            hit = true;
            closest = hit_info.t;
        }
    }
    
    

    
    for (int o = 0; o < trimesh_count; o++)
    {
        
        if (!hit_axis_alligned_box(trimesh_array[o].box, r))
        {
            continue;
        }

        

        for (int i = 0; i < triangle_count[o]; i++)
        {
            
            
            vec3 a = (/*trimesh_array[o].transform * */vec4(trimesh_array[o].vertices[trimesh_array[o].indices[i][0]][0], trimesh_array[o].vertices[trimesh_array[o].indices[i][0]][1], trimesh_array[o].vertices[trimesh_array[o].indices[i][0]][2], 1.0)).xyz;
            vec3 b = (/*trimesh_array[o].transform * */vec4(trimesh_array[o].vertices[trimesh_array[o].indices[i][1]][0], trimesh_array[o].vertices[trimesh_array[o].indices[i][1]][1], trimesh_array[o].vertices[trimesh_array[o].indices[i][1]][2], 1.0)).xyz;
            vec3 c = (/*trimesh_array[o].transform * */vec4(trimesh_array[o].vertices[trimesh_array[o].indices[i][2]][0], trimesh_array[o].vertices[trimesh_array[o].indices[i][2]][1], trimesh_array[o].vertices[trimesh_array[o].indices[i][2]][2], 1.0)).xyz;

             
            
            
            vec3 color = vec3(trimesh_array[o].color[0], trimesh_array[o].color[1], trimesh_array[o].color[2]);
            vec4 emission = vec4(trimesh_array[o].emission[0], trimesh_array[o].emission[1], trimesh_array[o].emission[2], trimesh_array[o].emission[3]);
            float reflection = trimesh_array[o].reflection;
            
            Triangle triangle = Triangle(a, b, c, 
            Material(color, emission.rbg, emission.w,reflection));
                    
      

            if (hit_triangle(triangle, r, 0, closest, hit_info))
            {
                hit = true;
                closest = hit_info.t;
            }
        }
    }
    

    return hit;
}

vec3 get_environment_light(Ray ray)
{
    //vec3 skyblue =    vec3(0.529f, 0.808f, 0.922f);
    //vec3 horisontti = vec3(.8f, .8f, .8f);         
    

    float gradient = pow(smoothstep(0.0, 0.4, -ray.dir.y), 0.35f);

    return mix(sky_color, horizont_color, gradient);
}

vec3 ray_color(Ray r, int max_bounce_count, inout uint seed)
{
    HitInfo hit_info;
    Ray ray = r;
    float multiplier = 1.f;
    
    vec3 incoming_light = vec3(0);
    vec3 ray_color = vec3(1);
    
    for (int i = 0; i < max_bounce_count+1; i++)
    {
        HitInfo hit_info;
        if (cast_ray(ray, hit_info))
        {
            ray.origin = hit_info.p + hit_info.normal * 0.0001f;
            vec3 refraction = random_hemisphere_dir(hit_info.normal, seed); // refraction
            vec3 reflection = ray.dir - 2 * (dot(ray.dir, hit_info.normal)) * hit_info.normal; // reflection 
            ray.dir = mix(refraction, reflection, hit_info.material.reflection_multiplier);
            vec3 emitted_light = hit_info.material.emission_color * hit_info.material.emission_strenght;

            incoming_light += emitted_light * ray_color;
            ray_color *= hit_info.material.color;
        }
        else
        {
            incoming_light += ray_color * get_environment_light(ray) * (1.f / float(pow(2, i)));
            break;
        }
    }
    return incoming_light;
}


vec3 calculate_pixel_color(vec3 pixel_color, float samples_per_pixel)
{
    float scale = 1.f / samples_per_pixel;
    pixel_color = sqrt(scale * pixel_color);
    return clamp(pixel_color, 0.f, 1.f);
}

void main()
{
    frag_color = vec4(0.f, 0.f, 0.f, 1.f);

    uint t = time;
    random(t);
    uint pixel_index = uint(gl_FragCoord.y * resolution.x + gl_FragCoord.x) + t;

    if (((uint(gl_FragCoord.y) + time) % fraction_pixel_per_frame) != 0)
    {
        return;
    }

    float aspect = float(resolution.x) / float(resolution.y);

    float viewport_height = 2.f;
    float viewport_width = aspect * viewport_height;
 //   float focal_length = 1.f;

    vec3 horizontal = vec3(viewport_width, 0, 0);
    vec3 vertical = vec3(0, viewport_height, 0);
    vec3 lower_left_corner = camera - horizontal * 0.5f - vertical * 0.5f - vec3(0, 0, focal_length);

    vec2 uv;
    vec3 pixel_color = vec3(0);
    
   
  

    random(pixel_index);
  
    /*
    mat3 rotz;
    rotz[0] = vec3(cos(angle), -sin(angle), 0);
    rotz[1] = vec3(sin(angle), cos(angle), 0);
    rotz[2] = vec3(0, 0, 1);
    */
    
    mat3 roty;
    roty[0] = vec3(cos(camera_rotation.y), 0, sin(camera_rotation.y));
    roty[1] = vec3(0, 1, 0);
    roty[2] = vec3(-sin(camera_rotation.y), 0, cos(camera_rotation.y));
    
    mat3 rotx;
    rotx[0] = vec3(1, 0, 0);
    rotx[1] = vec3(0, cos(camera_rotation.x), -sin(camera_rotation.x));
    rotx[2] = vec3(0, sin(camera_rotation.x), cos(camera_rotation.x));

    //uint(gl_FragCoord.y * resolution.x + gl_FragCoord.x) * 
    for (int i = 0; i < samples_per_pixel; i++)
    {
        uv.x = (gl_FragCoord.x + random(pixel_index)) / (resolution.x - 1);
        uv.y = (gl_FragCoord.y + random(pixel_index)) / (resolution.y - 1);

        vec3 dir = (lower_left_corner + uv.x * horizontal + uv.y * vertical - camera) * rotx * roty;
        Ray ray = Ray(camera, dir);
        pixel_color += ray_color(ray, bounces, pixel_index);
    }
    frag_color.rgb = calculate_pixel_color(pixel_color, samples_per_pixel);
    //FragColor.rgb = ray_color(ray);
}
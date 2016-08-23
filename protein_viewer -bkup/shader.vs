#version 330

in vec3 Position;

uniform mat4 gWorld;

out vec4 Color;

void main()
{
    gl_Position = gWorld  * vec4(0.06*Position, 1.0);
    
    //Color= vec4((0.06*Position.x+1.0f)/2,(0.06*Position.y+1.0f)/2,(0.06*Position.z+1.0f)/2,1.0);
    //Color= vec4((gl_Position.x+1.0f)/2,(gl_Position.y+1.0f)/2,(gl_Position.z+1.0f)/2,1.0);
    Color=-0.5*vec4(0.0,gl_Position.z-0.8,0.0f,gl_Position.w);
    //Color=vec4(1.0,1.0,1.0,1.0);
}

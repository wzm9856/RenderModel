#version 330 core

in vec3 Pos;

layout (location = 0) out float FragColor;
layout (location = 1) out vec3 Pos1;

uniform vec4 color;

void main()
{    
    FragColor = color.x*0.299 + color.y*0.587 + color.z*0.114;
    Pos1 = Pos;
}
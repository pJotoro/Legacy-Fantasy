#version 460

void main() {
	vec2 vertices[3] = {
	    vec2(0.5, 0.5),
		vec2(-0.5, 0.5),
	    vec2(0.0, -0.5),
	 };

	gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
}
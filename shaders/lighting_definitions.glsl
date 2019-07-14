void getIncomingLight(vec3 pos, out vec3 ambient, out vec3 direct);

vec3 getReflectedDirectLight(vec3 normal, vec3 incoming);
vec3 getReflectedAmbientLight(vec3 normal, vec3 incoming);

vec3 calcWaterEnvColor(vec3 ambientLight, vec3 directLight);
void calcLight(vec3 pos, vec3 normal, out vec3 direct, out vec3 ambient);
void calcLightWithDetail(vec3 pos, vec3 normal, vec3 normal_detail, out vec3 direct, out vec3 ambient);

vec3 calcLight(vec3 pos, vec3 normal, float direct_scale, float ambient_scale);

vec2 getHeightMapTextureCoords(in TerrainLayer layer, vec2 pos_m);
vec2 getNormalMapCoords(in TerrainLayer layer, vec2 pos_m);
vec3 sampleTerrainNormalMap(in TerrainLayer layer, vec2 pos_m);
float getDetailMapBlend(vec2 pos_m);
float getTerrainHeight(vec2 pos_m);

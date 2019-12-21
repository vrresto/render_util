void render_util::MapTextures::setForestMap(ImageGreyScale::ConstPtr image)
{
  auto texture = render_util::createTexture(image, true);

  TextureParameters<int> params;

  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  params.set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  params.set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
//   params.set(GL_TEXTURE_LOD_BIAS, 10.0);

  params.apply(texture);

  p->m_material->setTexture(TEXUNIT_FOREST_MAP, texture);
}


void render_util::MapTextures::setForestLayers(const std::vector<ImageRGBA::ConstPtr> &images)
{
  auto resampled = resampleImages(images, getMaxWidth(images));
  TexturePtr texture = createTextureArray<ImageRGBA>(resampled);

  TextureParameters<int> params;
  params.set(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  params.set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//   params.set(GL_TEXTURE_LOD_BIAS, 2.0);
  params.apply(texture);

  p->m_material->setTexture(TEXUNIT_FOREST_LAYERS, texture);
}

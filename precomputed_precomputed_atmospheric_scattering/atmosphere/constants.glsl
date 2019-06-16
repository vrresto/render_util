const int TRANSMITTANCE_TEXTURE_WIDTH = @transmittance_texture_width@;
const int TRANSMITTANCE_TEXTURE_HEIGHT = @transmittance_texture_height@;
const int SCATTERING_TEXTURE_R_SIZE = @scattering_texture_r_size@;
const int SCATTERING_TEXTURE_MU_SIZE = @scattering_texture_mu_size@;
const int SCATTERING_TEXTURE_MU_S_SIZE = @scattering_texture_mu_s_size@;
const int SCATTERING_TEXTURE_NU_SIZE = @scattering_texture_nu_size@;
const int IRRADIANCE_TEXTURE_WIDTH = @irradiance_texture_width@;
const int IRRADIANCE_TEXTURE_HEIGHT = @irradiance_texture_height@;

const AtmosphereParameters ATMOSPHERE = AtmosphereParameters(
          @solar_irradiance@,
          @sun_angular_radius@,
          @bottom_radius@,
          @top_radius@,
          @rayleigh_density@,
          @rayleigh_scattering@,
          @mie_density@,
          @mie_scattering@,
          @mie_extinction@,
          @mie_phase_function_g@,
          @absorption_density@,
          @absorption_extinction@,
          @ground_albedo@,
          @mu_s_min@);

const vec3 SKY_SPECTRAL_RADIANCE_TO_LUMINANCE = @sky_k@;
const vec3 SUN_SPECTRAL_RADIANCE_TO_LUMINANCE = @sun_k@;

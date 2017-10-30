#include <rendercat/util/color_temperature.hpp>
#include <glm/common.hpp>
#include <glm/gtc/color_space.hpp>

// based on MIT-licensed JS implementation from https://github.com/neilbartlett/color-temperature
glm::vec3 rc::util::temperature_to_srgb_color(float kelvin)
{
	float temperature = kelvin / 100.0f;
	float red, green, blue;

	if (temperature < 66.0f) {
		red = 255.0f;
	} else {
		// a + b x + c Log[x] /.
		// {a -> 351.97690566805693`,
		// b -> 0.114206453784165`,
		// c -> -40.25366309332127
		//x -> (kelvin/100) - 55}
		red = temperature - 55.0f;
		red = 351.97690566805693f + 0.114206453784165f * red - 40.25366309332127f * glm::log(red);
		red = glm::clamp(red, 0.0f, 255.0f);
	}

	/* Calculate green */
	if (temperature < 66.0f) {

		// a + b x + c Log[x] /.
		// {a -> -155.25485562709179`,
		// b -> -0.44596950469579133`,
		// c -> 104.49216199393888`,
		// x -> (kelvin/100) - 2}
		green = temperature - 2.0f;
		green = -155.25485562709179f - 0.44596950469579133f * green + 104.49216199393888f * glm::log(green);
		green = glm::clamp(green, 0.0f, 255.0f);

	} else {

		// a + b x + c Log[x] /.
		// {a -> 325.4494125711974`,
		// b -> 0.07943456536662342`,
		// c -> -28.0852963507957`,
		// x -> (kelvin/100) - 50}
		green = temperature - 50.0f;
		green = 325.4494125711974f + 0.07943456536662342f * green - 28.0852963507957f * glm::log(green);
		green = glm::clamp(green, 0.0f, 255.0f);

	}

	/* Calculate blue */
	if (temperature >= 66.0f) {
		blue = 255.0f;
	} else {

		if (temperature <= 20.0f) {
			blue = 0.0f;
		} else {

			// a + b x + c Log[x] /.
			// {a -> -254.76935184120902`,
			// b -> 0.8274096064007395`,
			// c -> 115.67994401066147`,
			// x -> kelvin/100 - 10}
			blue = temperature - 10.0f;
			blue = -254.76935184120902f + 0.8274096064007395f * blue + 115.67994401066147f * glm::log(blue);
			blue = glm::clamp(blue, 0.0f, 255.0f);
		}
	}

	return glm::vec3{red, green, blue};
}

glm::vec3 rc::util::temperature_to_linear_color(float kelvin)
{
	return glm::convertSRGBToLinear(temperature_to_srgb_color(kelvin) / glm::vec3{255.0f, 255.0f, 255.0f});
}

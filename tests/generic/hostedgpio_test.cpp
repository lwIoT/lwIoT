/*
 * Unit test for the hosted GPIO chip.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <lwiot.h>

#include <lwiot/hosted/hostedgpiochip.h>
#include <ostream>
#include <iostream>

int main(int argc, char **argv)
{
	lwiot_init();

	lwiot::HostedGpioChip hosted(10);

	hosted.write(1, true);
	hosted.write(2, true);
	lwiot_udelay(10);

	hosted.write(1, false);
	hosted.write(2, false);
	lwiot_udelay(10);

	hosted.write(1, false);
	hosted.write(2, true);
	lwiot_udelay(10);

	hosted.write(1, true);
	hosted.write(2, false);
	lwiot_udelay(10);

	hosted.write(1, false);
	hosted.write(2, true);
	lwiot_udelay(10);

	hosted.write(1, true);
	hosted.write(2, false);
	lwiot_udelay(10);

	hosted.write(2, true);
	hosted.write(1, true);

	std::cout << "Rendering plot.." << std::endl;

	lwiot_destroy();
	return -EXIT_SUCCESS;
}

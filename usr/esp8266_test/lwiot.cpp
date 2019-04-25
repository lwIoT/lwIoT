/*
 * ESP8266 basic unit test.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <esp8266.h>
#include <math.h>
#include <lwiot.h>

#include <lwiot/stl/string.h>
#include <lwiot/scopedlock.h>
#include <lwiot/log.h>
#include <lwiot/stream.h>
#include <lwiot/io/gpiochip.h>
#include <lwiot/io/gpiopin.h>
#include <lwiot/io/watchdog.h>

#include <lwiot/network/ipaddress.h>
#include <lwiot/network/wifistation.h>
#include <lwiot/network/wifiaccesspoint.h>

#include <lwiot/util/application.h>
#include <lwiot/util/measurementvector.h>

#include <lwiot/kernel/functionalthread.h>
#include <lwiot/kernel/thread.h>

#include <lwip/api.h>
#include <lwip/ip_addr.h>

#include <dhcpserver.h>

class EspTestApplication : public lwiot::Functor {
public:
	void run() override
	{
		bool value = false;

		print_dbg("Main thread started..\n");

		lwiot::GpioPin outPin = 5;
		outPin.mode(lwiot::PinMode::OUTPUT);
		wdt.enable(2000);

		while(true) {
			int i = 0;
			while(i++ < 20) {
				outPin.write(value);
				value = !value;

				wdt.reset();
				lwiot_sleep(50);
			}

			print_dbg("Ping!\n");
		}
	}
};

extern "C" void lwiot_setup()
{
	EspTestApplication runner;
	lwiot::Application app(runner);

	app.start();
}

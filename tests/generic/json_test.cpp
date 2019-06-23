/*
 * JSON unit test.
 * 
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <stdio.h>
#include <lwiot.h>

#include <lwiot/util/json.h>
#include <lwiot/stl/string.h>
#include <lwiot/test.h>


int main(int argc, char **argv)
{
	lwiot::DynamicJsonBuffer jbuffer;

	const char json[] = "{\"sensor\":\"gps\",\"time\":1351824120,\"data\":[48.756080,2.302038]}";

	lwiot::JsonObject& root = jbuffer.parseObject(json);
	const char* sensor = root["sensor"];
	long time          = root["time"];
	double latitude    = root["data"][0];
	double longitude   = root["data"][1];

	printf("JSON:\nSensor: %s\nTime: %li\nLatitude: %f\nLongitude: %f\n", sensor, time, latitude, longitude);

	auto& obj = jbuffer.createObject();
	auto& ary = jbuffer.createArray();

	ary.add(4);
	ary.add(9);
	ary.add(1);
	ary.add(10);

	obj["data"] = ary;

	lwiot::stl::String result;
	obj.prettyPrintTo(result);

	printf("JSON data:\n%s\n", result.c_str());
	wait_close();
	return -EXIT_SUCCESS;
}

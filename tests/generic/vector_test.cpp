/*
 * Vector unit test.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#include <stdlib.h>
#include <lwiot.h>
#include <assert.h>

#include <lwiot/log.h>
#include <lwiot/test.h>
#include <lwiot/stl/vector.h>
#include <lwiot/stl/array.h>

struct IteratorTest {
	void test_const_iter(const lwiot::stl::Vector<int>& vec) const
	{
		for(lwiot::stl::Vector<int>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter) {
			print_dbg("Entry: %i\n", *iter);
		}
	}

	void test_iter(lwiot::stl::Vector<int>& vec)
	{
		for(lwiot::stl::Vector<int>::iterator iter = vec.begin(); iter != vec.end(); ++iter) {
			print_dbg("Entry: %i\n", *iter);
		}
	}
};

int main(int argc, char**argv)
{
	lwiot_init();

	IteratorTest test;
	lwiot::stl::Vector<int> v1;
	lwiot::stl::Vector<int> v2;
	lwiot::stl::Vector<int> v3;

	UNUSED(argc);
	UNUSED(argv);

	v1.add(2);
	v1.add(21);
	v1.add(120);
	v1.pushback(1411);
	v1[3] = 510;

	v2 = v1;
	print_dbg("Iterator test:\n");
	test.test_iter(v2);

	print_dbg("Const iterator test:\n");
	test.test_const_iter(v1);

	assert(v2[1] == 21);
	assert(v2[0] == 2);
	assert(v2[3] == 510);

	lwiot_destroy();
	wait_close();
	return -EXIT_SUCCESS;
}

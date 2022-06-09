// ESA S2 product converter for cloud mask labeling and processing
//
// Copyright 2021 - 2022 KappaZeta Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "util/geometry.hpp"

std::vector<std::vector<unsigned char>> fill_poly_overlap(const AABB<int> &image_aabb, Polygon<int> &poly, float pixel_size_div, bool buffer_out);


class PolyFillTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(PolyFillTest);
CPPUNIT_TEST(testTiny01);
CPPUNIT_TEST(testTinyEdge01);
CPPUNIT_TEST(testTinyEdge02);
CPPUNIT_TEST(testArb01);
CPPUNIT_TEST_SUITE_END();

	public:
		void setUp() {
		}

		void tearDown() {
		}

		void print_mask(std::vector<std::vector<unsigned char>> &subtile_mask) {
			Vector<int> p;
			for (p.y=0; p.y<(int)subtile_mask.size(); p.y++) {
				for (p.x=0; p.x<(int)subtile_mask[p.y].size(); p.x++) {
					std::cout << (char) ('0' + subtile_mask[p.x][p.y]) << " ";
				}
				std::cout << std::endl;
			}
		}

		void print_mask_cmp(std::vector<std::vector<unsigned char>> &subtile_mask, const char *expected) {
			unsigned int total = 0;
			Vector<int> p;

			std::cout << "Expected:" << std::endl;
			for (p.y=0; p.y<(int)subtile_mask.size(); p.y++) {
				for (p.x=0; p.x<(int)subtile_mask[p.y].size(); p.x++) {
					std::cout << (char) (expected[total]) << " ";
					total++;
				}
				std::cout << std::endl;
			}

			std::cout << "Result:" << std::endl;
			print_mask(subtile_mask);
		}

		void check_mask(std::vector<std::vector<unsigned char>> &subtile_mask, const char *expected) {
			unsigned int total = 0, correct = 0;
			Vector<int> p;
			for (p.y=0; p.y<(int)subtile_mask.size(); p.y++) {
				for (p.x=0; p.x<(int)subtile_mask[p.y].size(); p.x++) {
					if ('0' + subtile_mask[p.x][p.y] == expected[total])
						correct++;
					total++;
				}
			}

			if (total != correct) {
				print_mask_cmp(subtile_mask, expected);
			}
			CPPUNIT_ASSERT(total == correct);
		}

		void testArb01() {
			std::vector<std::vector<unsigned char>> subtile_mask;
			AABB<int> image_aabb(0, 0, 10980, 10980);
			Polygon<int> poly({
				Vector<int>(7296, 1104),
				Vector<int>(7632, 3184),
				Vector<int>(9600, 4928),
				Vector<int>(9088, 7936),
				Vector<int>(8832, 5440),
				Vector<int>(7360, 4704),
				Vector<int>(5952, 4560),
				Vector<int>(4528, 4912),
				Vector<int>(5536, 6944),
				Vector<int>(3424, 5216),
				Vector<int>(3264, 7440),
				Vector<int>(2880, 3680),
				Vector<int>(6048, 3200)
			});

			subtile_mask = fill_poly_overlap(image_aabb, poly, 512, true);

			Vector<int> p;
			for (p.y=0; p.y<(int)subtile_mask.size(); p.y++) {
				for (p.x=0; p.x<(int)subtile_mask[p.y].size(); p.x++) {
					std::cout << (char) ('0' + subtile_mask[p.x][p.y]) << " ";
				}
				std::cout << std::endl;
			}

			CPPUNIT_ASSERT(1);
		}

		void testTiny01() {
			// Grid of 4 tiles, with a tiny polygon inside a single tile.
			std::vector<std::vector<unsigned char>> subtile_mask;
			const char expected[2][2] = {
				'0', '0',
				'0', '1'
			};
			AABB<int> image_aabb(0, 0, 1024, 1024);
			Polygon<int> poly({
				Vector<int>(732, 748),
				Vector<int>(844, 760),
				Vector<int>(838, 841),
				Vector<int>(721, 825)
			});

			subtile_mask = fill_poly_overlap(image_aabb, poly, 512, false);

			check_mask(subtile_mask, (const char*) expected);
		}

		void testTinyEdge01() {
			// Grid of 4 tiles, with a tiny polygon crossing the horizontal line.
			std::vector<std::vector<unsigned char>> subtile_mask;
			const char expected[2][2] = {
				'0', '1',
				'0', '1'
			};
			AABB<int> image_aabb(0, 0, 1024, 1024);
			Polygon<int> poly({
				Vector<int>(720, 469),
				Vector<int>(837, 495),
				Vector<int>(807, 630),
				Vector<int>(688, 607)
			});

			subtile_mask = fill_poly_overlap(image_aabb, poly, 512, false);

			check_mask(subtile_mask, (const char*) expected);
		}

		void testTinyEdge02() {
			// Grid of 4 tiles, with a tiny polygon crossing the vertical line.
			std::vector<std::vector<unsigned char>> subtile_mask;
			const char expected[2][2] = {
				'0', '0',
				'1', '1'
			};
			AABB<int> image_aabb(0, 0, 1024, 1024);
			Polygon<int> poly({
				Vector<int>(313, 703),
				Vector<int>(844, 760),
				Vector<int>(838, 841),
				Vector<int>(307, 771)
			});

			subtile_mask = fill_poly_overlap(image_aabb, poly, 512, false);

			check_mask(subtile_mask, (const char*) expected);
		}
};


class ClipAABBTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(ClipAABBTest);
CPPUNIT_TEST(testSubset01);
CPPUNIT_TEST(testSubset02);
CPPUNIT_TEST(testSubset03);
CPPUNIT_TEST(testAll);
CPPUNIT_TEST(testNone);
CPPUNIT_TEST_SUITE_END();

	public:
		void setUp() {
		}

		void tearDown() {
		}

		void compareIPolys(Polygon<int> &poly_expected, Polygon<int> &poly) {
			CPPUNIT_ASSERT(poly.size() == poly_expected.size());
			for (size_t i=0; i<poly.size(); i++) {
				CPPUNIT_ASSERT(poly[i] == poly_expected[i]);
			}
		}

		void testSubset01() {
			// Small AABB inside a large polygon.
			AABB<int> clip_aabb(50, 50, 80, 80);
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});
			Polygon<int> poly_expected({
				Vector<int>(50, 50),
				Vector<int>(80, 50),
				Vector<int>(80, 80),
				Vector<int>(50, 80)
			});

			poly.clip_to_aabb(clip_aabb);

			compareIPolys(poly_expected, poly);
		}

		void testSubset02() {
			// Small AABB on the edge of a large polygon.
			AABB<int> clip_aabb(80, 30, 120, 120);
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});
			Polygon<int> poly_expected({
				Vector<int>(80, 30),
				Vector<int>(100, 30),
				Vector<int>(100, 104),
				Vector<int>(80, 104)
			});

			poly.clip_to_aabb(clip_aabb);

			compareIPolys(poly_expected, poly);
		}

		void testSubset03() {
			// Small AABB on the corner of a large polygon.
			AABB<int> clip_aabb(80, 80, 140, 140);
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});
			Polygon<int> poly_expected({
				Vector<int>(80, 80),
				Vector<int>(100, 80),
				Vector<int>(100, 104),
				Vector<int>(80, 104)
			});

			poly.clip_to_aabb(clip_aabb);

			compareIPolys(poly_expected, poly);
		}

		void testAll() {
			// Polygon is already inside the AABB.
			AABB<int> clip_aabb(0, 0, 140, 140);
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});
			Polygon<int> poly_expected({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});

			poly.clip_to_aabb(clip_aabb);

			compareIPolys(poly_expected, poly);
		}

		void testNone() {
			// Polygon is outside the AABB.
			AABB<int> clip_aabb(140, 140, 150, 150);
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 2),
				Vector<int>(100, 104),
				Vector<int>(0, 104)
			});

			poly.clip_to_aabb(clip_aabb);
			std::cout << poly << " " << poly.area() << std::endl;

			CPPUNIT_ASSERT(poly.area() <= 0.001);
		}
};

class PolyAreaTest: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(PolyAreaTest);
CPPUNIT_TEST(testSimple01);
CPPUNIT_TEST(testPoint01);
CPPUNIT_TEST(testEmpty);
CPPUNIT_TEST_SUITE_END();

	public:
		void setUp() {
		}

		void tearDown() {
		}

		void testSimple01() {
			Polygon<int> poly({
				Vector<int>(0, 0),
				Vector<int>(100, 0),
				Vector<int>(100, 100),
				Vector<int>(0, 100)
			});
			std::cout << poly << " " << poly.area() << std::endl;

			CPPUNIT_ASSERT(poly.area() == 10000);
		}

		void testPoint01() {
			Polygon<int> poly({
				Vector<int>(10, 10),
				Vector<int>(10, 10),
				Vector<int>(10, 10),
				Vector<int>(10, 10)
			});
			std::cout << poly << " " << poly.area() << std::endl;

			CPPUNIT_ASSERT(poly.area() <= 0.001);
		}

		void testEmpty() {
			Polygon<int> poly;
			std::cout << poly << " " << poly.area() << std::endl;

			CPPUNIT_ASSERT(poly.area() <= 0.001);
		}
};

int main(int argc, char* argv[]) {
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(PolyFillTest::suite());
	runner.addTest(ClipAABBTest::suite());
	runner.addTest(PolyAreaTest::suite());
	runner.run();

	return 0;
}


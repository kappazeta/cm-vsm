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

#include "util/text.hpp"
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

			CPPUNIT_ASSERT(poly.area() == 10000);
		}

		void testPoint01() {
			Polygon<int> poly({
				Vector<int>(10, 10),
				Vector<int>(10, 10),
				Vector<int>(10, 10),
				Vector<int>(10, 10)
			});

			CPPUNIT_ASSERT(poly.area() <= 0.001);
		}

		void testEmpty() {
			Polygon<int> poly;

			CPPUNIT_ASSERT(poly.area() <= 0.001);
		}
};

class TestSubtileCoords: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(TestSubtileCoords);
CPPUNIT_TEST(testSplitStrEmpty01);
CPPUNIT_TEST(testSplitStr01);
CPPUNIT_TEST(testExtractCoords01);
CPPUNIT_TEST(testExtractCoords02);
CPPUNIT_TEST(testExtractCoordsEmpty01);
CPPUNIT_TEST(testFillWhole01);
CPPUNIT_TEST(testApplyMask01);
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

		void testSplitStrEmpty01() {
			std::vector<std::string> tokens = split_str("", ',');
			CPPUNIT_ASSERT(tokens.size() == 0);
		}

		void testSplitStr01() {
			std::vector<std::string> tokens = split_str("1,2 3,4", ' ');
			CPPUNIT_ASSERT(tokens.size() == 2);
			CPPUNIT_ASSERT(tokens[0] == "1,2");
			CPPUNIT_ASSERT(tokens[1] == "3,4");
		}

		void testExtractCoords01() {
			std::vector<Vector<int>> coords = extract_coords("1_12,23_1", ',', '_');

			CPPUNIT_ASSERT(coords.size() == 2);
			CPPUNIT_ASSERT(coords[0].x == 1 && coords[0].y == 12);
			CPPUNIT_ASSERT(coords[1].x == 23 && coords[1].y == 1);
		}

		void testExtractCoords02() {
			std::vector<Vector<int>> coords = extract_coords("13_5,23_", ',', '_');

			CPPUNIT_ASSERT(coords.size() == 1);
			CPPUNIT_ASSERT(coords[0].x == 13 && coords[0].y == 5);
		}

		void testExtractCoordsEmpty01() {
			std::vector<Vector<int>> coords = extract_coords("", ',', '_');

			CPPUNIT_ASSERT(coords.size() == 0);
		}

		void testFillWhole01() {
			std::vector<std::vector<unsigned char>> subtile_mask;
			subtile_mask = fill_whole(AABB<int>(0, 0, 64, 100), 1, 0);

			unsigned int i, j, num = 0;
			// Count subtiles with value 0.
			for (i=0; i<subtile_mask.size(); i++) {
				for (j=0; j<subtile_mask[0].size(); j++) {
					if (subtile_mask[i][j] == 0)
						num++;
				}
			}
			CPPUNIT_ASSERT(num == 6400);
			CPPUNIT_ASSERT(i == 64);
			CPPUNIT_ASSERT(j == 100);
		}

		void testApplyMask01() {
			std::vector<std::vector<unsigned char>> a, b, c;
			a = fill_whole(AABB<int>(0, 0, 3, 3), 1, 0);
			b = fill_whole(AABB<int>(0, 0, 3, 3), 1, 0);

			a[1][0] = 1;
			a[1][1] = 1;
			a[1][2] = 1;
			a[0][1] = 1;
			a[1][1] = 1;
			a[2][1] = 1;

			b[0][0] = 1;
			b[1][1] = 1;
			b[2][2] = 1;
			b[1][0] = 1;

			c = apply_mask(a, b);

			unsigned int i, j, num = 0;
			// Count subtiles with value 1.
			for (i=0; i<c.size(); i++) {
				for (j=0; j<c[0].size(); j++) {
					if (c[i][j] == 1)
						num++;
				}
			}

			CPPUNIT_ASSERT(num == 2);
			CPPUNIT_ASSERT(c[1][0] == 1);
			CPPUNIT_ASSERT(c[1][1] == 1);
		}
};

int main(int argc, char* argv[]) {
	CppUnit::TextUi::TestRunner runner;

	runner.addTest(PolyFillTest::suite());
	runner.addTest(ClipAABBTest::suite());
	runner.addTest(PolyAreaTest::suite());
	runner.addTest(TestSubtileCoords::suite());
	runner.run();

	return 0;
}


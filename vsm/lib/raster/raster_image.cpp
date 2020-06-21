#include "raster/raster_image.hpp"


RasterImage::RasterImage(): main_depth(0), main_num_components(0), subset(nullptr) {}
RasterImage::~RasterImage() {
	clear();
}

void RasterImage::clear() {
	if (subset != nullptr) {
		delete subset;
		subset = nullptr;
	}
}

std::ostream& operator<<(std::ostream &out, const RasterImage& img) {
	if (img.subset != nullptr) {
		Magick::Geometry geom = img.subset->size();
		return out << "RasterImage(x0=" << geom.xOff()
			<< ", y0=" << geom.yOff()
			<< ", w=" << geom.width()
			<< ", h=" << geom.height()
			<< ", c=" << img.main_num_components
			<< ", d=" << img.subset->depth() << ")";
	}

	return out << "RasterImage()";
}

bool RasterImage::save(const std::filesystem::path &path) {
	if (subset != nullptr) {
		subset->write(path.string());
		return true;
	}
	return false;
}

bool RasterImage::scale(float f, bool point_filter) {
	if (subset != nullptr) {
		// No scaling needed?
		if (f >= 0.999f && f <= 1.001f)
			return true;

		Magick::Geometry geom_orig = subset->size();
		Magick::Geometry geom_new(geom_orig.width() * f, geom_orig.height() * f);
		if (point_filter)
			subset->filterType(Magick::PointFilter);
		else
			subset->filterType(Magick::SincFilter);
		subset->resize(geom_new);
		return true;
	}
	return false;
}

void RasterImage::remap_values(const unsigned char *values) {
	//! \todo Implement support for remapping colors.
	if (subset != nullptr) {
		unsigned int w = subset->columns();
		unsigned int h = subset->rows();
		unsigned int size = w * h;

		Magick::PixelPacket *px = subset->getPixels(0, 0, w, h);

		register double src_val, dst_val;

		for (unsigned int i=0; i<size; i++) {
			src_val = Magick::ColorGray(px[i]).shade();
			dst_val = values[(unsigned char) (255 * src_val)] / 255.0;
			px[i] = Magick::ColorGray(dst_val);
		}

		subset->syncPixels();
	}
}


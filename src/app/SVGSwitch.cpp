#include "app.hpp"


namespace rack {


SVGSwitch::SVGSwitch() {
	sw = new SVGWidget();
	addChild(sw);
}

void SVGSwitch::addFrame(std::shared_ptr<SVG> svg) {
	frames.push_back(svg);
	// If this is our first frame, automatically set SVG and size
	if (!sw->svg) {
		sw->setSVG(svg);
		box.size = sw->box.size;
	}
}

void SVGSwitch::step() {
	FramebufferWidget::step();
}

void SVGSwitch::onChange(EventChange &e) {
	assert(frames.size() > 0);
	int index = clampi((int) roundf(value), 0, frames.size() - 1);
	sw->setSVG(frames[index]);
	dirty = true;
	Switch::onChange(e);
}


} // namespace rack

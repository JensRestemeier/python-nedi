#include <Python.h>

#include <cmath>
#include <algorithm>

#include <Eigen/Core>
#include <Eigen/Dense>

enum WrapMode {
	WrapMode_Clamp,
	WrapMode_Wrap,
	WrapMode_Mirror,

	WrapMode_Max
};

float gamma[256];

static float toGamma(float c)
{
	if (c <= 0.0031308f)
	{
		return c * 12.92f;
	}
	else
	{
		return 1.055f * pow(c, 1.0f / 2.4f) - 0.055f;
	}
}

static float toLinear(float c)
{
	if (c <= 0.04045f)
	{
		return c / 12.92f;
	}
	else
	{
		return pow((c + 0.055f) / 1.055f, 2.4f);
	}
}

static void nedi_init()
{
	for (int i = 0; i < 256; i++)
	{
		gamma[i] = toLinear((float)i / 255.0f);
	}
}

static float clamp(float v, float lo, float hi)
{
	if (v < lo) {
		return lo;
	}
	else if (v > hi) {
		return hi;
	}
	else {
		return v;
	}
}

// to avoid compilcated clamping/wrapping per pixel I duplicate enough pixel to cover the filter kernel around the border
static void fixBorders(Eigen::ArrayXXf & work, WrapMode xWrap, WrapMode yWrap, int border, int xofs, int yofs)
{
	// fill in border
	int inHeight = work.rows() / 2 - border * 2;
	int inWidth = work.cols() / 2 - border * 2;
	switch (yWrap) {
	case WrapMode_Wrap:
		for (int y = 0; y < border; y++) {
			int ya1 = y * 2 + yofs;
			int ya2 = (y + inHeight + border) * 2 + yofs;
			int yb1 = ((y + inHeight - border) % inHeight + border) * 2 + yofs;
			int yb2 = (y % inHeight + border) * 2 + yofs;
			for (int x = 0; x < inWidth; x++) {
				int xa = (x + border) * 2 + xofs;
				work(ya1, xa) = work(yb1, xa);
				work(ya2, xa) = work(yb2, xa);
			}
		}
		break;
	case WrapMode_Clamp:
		for (int y = 0; y < border; y++) {
			int ya1 = y * 2 + yofs;
			int ya2 = (y + inHeight + border) * 2 + yofs;
			int yb1 = border * 2 + yofs;
			int yb2 = (inHeight + border - 1) * 2 + yofs;
			for (int x = 0; x < inWidth; x++) {
				int xa = (x + border) * 2 + xofs;
				work(ya1, xa) = work(yb1, xa);
				work(ya2, xa) = work(yb2, xa);
			}
		}
		break;
	case WrapMode_Mirror:
		for (int y = 0; y < border; y++) {
			int ya1 = y * 2 + yofs;
			int ya2 = (y + inHeight + border) * 2 + yofs;
			int yb1 = ((border - y + inHeight) % inHeight + border) * 2 - yofs;
			int yb2 = ((inHeight - y - 1) % inHeight + border) * 2 - yofs;
			for (int x = 0; x < inWidth; x++) {
				int xa = (x + border) * 2 + xofs;
				work(ya1, xa) = work(yb1, xa);
				work(ya2, xa) = work(yb2, xa);
			}
		}
		break;
	}
	switch (xWrap) {
	case WrapMode_Wrap:
		for (int x = 0; x < border; x++) {
			int xa1 = x * 2 + xofs;
			int xa2 = (x + inWidth + border) * 2 + xofs;
			int xb1 = ((x + inWidth - border) % inWidth + border) * 2 + xofs;
			int xb2 = (x % inWidth + border) * 2 + xofs;
			for (int y = 0; y < inHeight; y++) {
				int ya = (y + border) * 2 + yofs;
				work(ya, xa1) = work(ya, xb1);
				work(ya, xa2) = work(ya, xb2);
			}
		}
		break;
	case WrapMode_Clamp:
		for (int x = 0; x < border; x++) {
			int xa1 = x * 2 + xofs;
			int xa2 = (x + inWidth + border) * 2 + xofs;
			int xb1 = border * 2 + xofs;
			int xb2 = (inWidth + border - 1) * 2 + xofs;
			for (int y = 0; y < inHeight; y++) {
				int ya = (y + border) * 2 + yofs;
				work(ya, xa1) = work(ya, xb1);
				work(ya, xa2) = work(ya, xb2);
			}
		}
		break;
	case WrapMode_Mirror:
		for (int x = 0; x < border; x++) {
			int xa1 = x * 2 + xofs;
			int xa2 = (x + inWidth + border) * 2 + xofs;
			int xb1 = ((border - x + inWidth) % inWidth + border) * 2 - xofs;
			int xb2 = ((inWidth - x - 1) % inWidth + border) * 2 - xofs;
			for (int y = 0; y < inHeight; y++) {
				int ya = (y + border) * 2 + yofs;
				work(ya, xa1) = work(ya, xb1);
				work(ya, xa2) = work(ya, xb2);
			}
		}
		break;
	}
	for (int x = 0; x < border; x++) {
		int xa1 = x * 2 + xofs;
		int xa2 = (x + inWidth + border) * 2 + xofs;
		int xb1, xb2;
		switch (xWrap) {
		case WrapMode_Clamp:
			xb1 = border * 2 + xofs;
			xb2 = (inWidth + border - 1) * 2 + xofs;
			break;
		case WrapMode_Wrap:
			xb1 = ((x - border + inWidth) % inWidth + border) * 2 + xofs;
			xb2 = (x % inWidth + border) * 2 + xofs;
			break;
		case WrapMode_Mirror:
			xb1 = ((border - x + inWidth) % inWidth + border) * 2 - xofs;
			xb2 = ((inWidth - x - 1) % inWidth + border) * 2 - xofs;
			break;
		}
		for (int y = 0; y < border; y++) {
			int ya1 = y * 2 + yofs;
			int ya2 = (y + inHeight + border) * 2 + yofs;
			int yb1, yb2;
			switch (yWrap) {
			case WrapMode_Clamp:
				yb1 = border * 2 + yofs;
				yb2 = (inHeight + border - 1) * 2 + yofs;
				break;
			case WrapMode_Wrap:
				yb1 = ((y - border + inHeight) % inHeight + border) * 2 + yofs;
				yb2 = (y % inHeight + border) * 2 + yofs;
				break;
			case WrapMode_Mirror:
				yb1 = ((border - y + inHeight) % inHeight + border) * 2 - yofs;
				yb2 = ((inHeight - y - 1) % inHeight + border) * 2 - yofs;
				break;
			}
			work(ya1, xa1) = work(yb1, xb1);
			work(ya2, xa2) = work(yb2, xb2);
			work(ya2, xa1) = work(yb2, xb1);
			work(ya1, xa2) = work(yb1, xb2);
		}
	}
}

static void normalise_weights(Eigen::Vector4d & a)
{
	double a_min = 0.0;
	for (int i = 0; i < 4; i++) {
		if (a_min > a(i)) {
			a_min = a(i);
		}
	}
	double s = 0.0;
	for (int i = 0; i < 4; i++) {
		double w = a(i) - a_min;
		a(i) = w;
		s += w;
	}
	if (s > 0.0) {
		double oos = 1.0 / s;
		for (int i = 0; i < 4; i++) {
			a(i) *= oos;
		}
	}
	else {
		for (int i = 0; i < 4; i++) {
			a(i) = 0.25;
		}
	}
}

static Eigen::Vector4d solve(const Eigen::MatrixXd & C, const Eigen::VectorXd & r)
{
	Eigen::MatrixXd Ct = C.transpose();
	Eigen::Matrix4d CtxC = Ct * C;
	if (fabs(CtxC.determinant()) > std::numeric_limits<double>::epsilon()) {
		Eigen::Matrix4d CtxCI = CtxC.inverse();
		Eigen::Vector4d a = CtxCI * Ct * r;
		normalise_weights(a);
		return a;
	}
	else {
		return Eigen::Vector4d(0.25, 0.25, 0.25, 0.25);
	}
}

static void nedi_diag(Eigen::ArrayXXf & work_r, Eigen::ArrayXXf & work_g, Eigen::ArrayXXf & work_b, Eigen::ArrayXXf & work_a, int dx, int dy, int width, int height)
{
	// half kernel size and kernel size
	const int M2 = 3;
	const int M = M2 * 2;
	const int border = M + 1;

	const Eigen::Vector4f rgba_to_float(0.2126f * 0.75f, 0.7152f * 0.75f, 0.0722f * 0.75f, 0.25f);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Eigen::MatrixXd C(M * M, 4);
			Eigen::VectorXd r(M * M);

			// calculate weights (based on L*)
			for (int ny = 0; ny < M; ny++) {
				for (int nx = 0; nx < M; nx++) {
					int xx = x + nx - M2 + dx;
					int yy = y + ny - M2 + dy;
					for (int iy = 0; iy < 2; iy++) {
						for (int ix = 0; ix < 2; ix++) {
							int ixx = xx + (ix * 2) - 1;
							int iyy = yy + (iy * 2) - 1;

							int xpos = (ixx + border) * 2;
							int ypos = (iyy + border) * 2;
							Eigen::Vector4f col(work_r(ypos, xpos), work_g(ypos, xpos), work_b(ypos, xpos), work_a(ypos, xpos));
							C(ny * M + nx, iy * 2 + ix) = col.dot(rgba_to_float);
						}
					}

					int xpos = (xx + border) * 2;
					int ypos = (yy + border) * 2;
					Eigen::Vector4f col(work_r(ypos, xpos), work_g(ypos, xpos), work_b(ypos, xpos), work_a(ypos, xpos));
					r(ny*M + nx) = col.dot(rgba_to_float);
				}
			}

			Eigen::Vector4d a = solve(C, r);

			// interpolate (all channels with the same weight)
			for (int c = 0; c < 4; c++) {
				Eigen::Vector4f v(0.0f, 0.0f, 0.0f, 0.0f);
				for (int my = 0; my < 2; my++) {
					for (int mx = 0; mx < 2; mx++) {
						int xpos = (x + mx + border) * 2 + dx - 1;
						int ypos = (y + my + border) * 2 + dy - 1;
						v(0) += (float)(work_r(ypos, xpos) * a(my * 2 + mx));
						v(1) += (float)(work_g(ypos, xpos) * a(my * 2 + mx));
						v(2) += (float)(work_b(ypos, xpos) * a(my * 2 + mx));
						v(3) += (float)(work_a(ypos, xpos) * a(my * 2 + mx));
					}
				}
				int xpos = (x + border) * 2 + dx;
				int ypos = (y + border) * 2 + dy;
				work_r(ypos, xpos) = v(0);
				work_g(ypos, xpos) = v(1);
				work_b(ypos, xpos) = v(2);
				work_a(ypos, xpos) = v(3);
			}
		}
	}
}

static void nedi_cross(Eigen::ArrayXXf & work_r, Eigen::ArrayXXf & work_g, Eigen::ArrayXXf & work_b, Eigen::ArrayXXf & work_a, int dx, int dy, int width, int height)
{
	// half kernel size and kernel size
	const int M2 = 3;
	const int M = M2 * 2;
	const int border = M + 1;

	const Eigen::Vector4f rgba_to_float(0.2126f * 0.75f, 0.7152f * 0.75f, 0.0722f * 0.75f, 0.25f);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Eigen::MatrixXd C(M*M, 4);
			Eigen::VectorXd r(M*M);

			// calculate weights
			for (int ny = 0; ny < M; ny++) {
				for (int nx = 0; nx < M; nx++) {
					int xx = x * 2 + nx + ny - (M2 - 1) * 2 + dx - 1;
					int yy = y * 2 - nx + ny + dy;
					for (int iy = 0; iy < 2; iy++) {
						for (int ix = 0; ix < 2; ix++) {
							int ixx = xx + (ix + iy - 1) * 2;
							int iyy = yy + (-ix + iy) * 2;

							int xpos = ixx + border * 2;
							int ypos = iyy + border * 2;

							Eigen::Vector4f col(work_r(ypos, xpos), work_g(ypos, xpos), work_b(ypos, xpos), work_a(ypos, xpos));
							C(ny * M + nx, iy * 2 + ix) = col.dot(rgba_to_float);
						}
					}
					int xpos = xx + border * 2;
					int ypos = yy + border * 2;
					Eigen::Vector4f col(work_r(ypos, xpos), work_g(ypos, xpos), work_b(ypos, xpos), work_a(ypos, xpos));
					r(ny*M + nx) = col.dot(rgba_to_float);
				}
			}

			Eigen::Vector4d a = solve(C, r);

			// interpolate
			for (int c = 0; c < 4; c++) {
				Eigen::Vector4f v(0.0f, 0.0f, 0.0f, 0.0f);
				for (int my = 0; my < 2; my++) {
					for (int mx = 0; mx < 2; mx++) {
						int xx = x * 2 + mx + my - 1;
						int yy = y * 2 - mx + my;
						int ypos = yy + border * 2 + dy;
						int xpos = xx + border * 2 + dx;
						v(0) += (float)(work_r(ypos, xpos) * a(my * 2 + mx));
						v(1) += (float)(work_g(ypos, xpos) * a(my * 2 + mx));
						v(2) += (float)(work_b(ypos, xpos) * a(my * 2 + mx));
						v(3) += (float)(work_a(ypos, xpos) * a(my * 2 + mx));
					}
				}
				int xpos = (x + border) * 2 + dx;
				int ypos = (y + border) * 2 + dy;
				work_r(ypos, xpos) = v(0);
				work_g(ypos, xpos) = v(1);
				work_b(ypos, xpos) = v(2);
				work_a(ypos, xpos) = v(3);
			}
		}
	}
}

static void nedi_scale(uint8_t * src, uint8_t * dst, int width, int height, WrapMode xWrap, WrapMode yWrap)
{
	// half kernel size and kernel size
	const int M2 = 3;
	const int M = M2 * 2;
	const int border = M + 1;

	// special work image (slightly bigger to remove border checks and to easily support wrapping semantics)
	int work_width = (width + border * 2) * 2;
	int work_height = (height + border * 2) * 2;
	Eigen::ArrayXXf work_r(work_height, work_width);
	Eigen::ArrayXXf work_g(work_height, work_width);
	Eigen::ArrayXXf work_b(work_height, work_width);
	Eigen::ArrayXXf work_a(work_height, work_width);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			uint32_t ofs = (y * width + x) * 4;
			work_r((y + border) * 2, (x + border) * 2) = gamma[src[ofs + 0]];
			work_g((y + border) * 2, (x + border) * 2) = gamma[src[ofs + 1]];
			work_b((y + border) * 2, (x + border) * 2) = gamma[src[ofs + 2]];
			work_a((y + border) * 2, (x + border) * 2) = src[ofs + 3] / 255.0f;
		}
	}

	// copy the correct pixel into the border region
	fixBorders(work_r, xWrap, yWrap, border, 0, 0);
	fixBorders(work_g, xWrap, yWrap, border, 0, 0);
	fixBorders(work_b, xWrap, yWrap, border, 0, 0);
	fixBorders(work_a, xWrap, yWrap, border, 0, 0);

	// diagonal pass
	nedi_diag(work_r, work_g, work_b, work_a, 1, 1, width, height);
#if 0
	// copy the correct pixel into the border region
	fixBorders(work, xWrap, yWrap, border, 0, 0);

	// diagonal pass 2
	nedi_diag(work, 0, 0, width, height);
#endif
	// copy the new pixel into the border region
	fixBorders(work_r, xWrap, yWrap, border, 1, 1);
	fixBorders(work_g, xWrap, yWrap, border, 1, 1);
	fixBorders(work_b, xWrap, yWrap, border, 1, 1);
	fixBorders(work_a, xWrap, yWrap, border, 1, 1);

	// horizontal pass
	nedi_cross(work_r, work_g, work_b, work_a, 1, 0, width, height);

	// vertical pass
	nedi_cross(work_r, work_g, work_b, work_a, 0, 1, width, height);

	// crop the final data out of the work image
	for (int y = 0; y < height * 2; y++) {
		for (int x = 0; x < width * 2; x++) {
			uint32_t ofs = (y * width * 2 + x) * 4;
			dst[ofs + 0] = (int)std::round(clamp(toGamma(work_r(y + border * 2, x + border * 2)) * 255.0f, 0.0f, 255.0f));
			dst[ofs + 1] = (int)std::round(clamp(toGamma(work_g(y + border * 2, x + border * 2)) * 255.0f, 0.0f, 255.0f));
			dst[ofs + 2] = (int)std::round(clamp(toGamma(work_b(y + border * 2, x + border * 2)) * 255.0f, 0.0f, 255.0f));
			dst[ofs + 3] = (int)std::round(clamp(work_a(y + border * 2, x + border * 2) * 255.0f, 0.0f, 255.0f));
		}
	}
}

#define PY3K (PY_VERSION_HEX >= 0x03000000)

static PyObject*
py_nedi(PyObject* self, PyObject* args) {
	Py_buffer src;
	int width;
	int height;

	if (PyArg_ParseTuple(args, "y*ii", &src, &width, &height)) {
		int scale = 2;
		int dest_width = width * scale;
		int dest_height = height * scale;
		int dest_buffer_size = dest_width * dest_height * 4;
		uint8_t * dest_buffer = new uint8_t[dest_buffer_size];

		nedi_scale((uint8_t*)src.buf, dest_buffer, width, height, WrapMode_Clamp, WrapMode_Clamp);
		PyBuffer_Release(&src);

		PyObject * _dest = PyBytes_FromStringAndSize((const char*)dest_buffer, dest_buffer_size);
		PyObject * _dest_width = PyLong_FromUnsignedLong(dest_width);
		PyObject * _dest_height = PyLong_FromUnsignedLong(dest_height);
		delete[] dest_buffer;

		return PyTuple_Pack(3, _dest, _dest_width, _dest_height);
	}
	Py_RETURN_NONE;
}

static PyMethodDef native_nedi_methods[] = {
	{ "py_nedi", (PyCFunction)py_nedi, METH_VARARGS, "NEDI algorithm" },
	{ NULL, NULL, 0, NULL }
};

#if PY3K
static PyModuleDef native_nedi_module = {

	PyModuleDef_HEAD_INIT,
	"_nedi",
	"New Edge-Directed Interpolation",
	-1,
	native_nedi_methods, NULL, NULL, NULL, NULL
};
#endif

extern "C" {
	PyMODINIT_FUNC
#if PY3K
		PyInit__nedi(void)
#else
		initnative_nedi(void)
#endif
	{
		PyObject* mainmod = PyImport_AddModule("__main__");
		PyObject* maindict = PyModule_GetDict(mainmod);

		PyObject* m;

#if PY3K
		m = PyModule_Create(&native_nedi_module);
#else
		m = Py_InitModule3("_nedi", native_nedi_methods, "New Edge-Directed Interpolation");
#endif
		if (m == NULL)
#if PY3K
			return NULL;
#else
			return;
#endif

		nedi_init();

#if PY3K
		return m;
#endif
	}
}

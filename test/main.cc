#include "nuklear_lib.h"
#include "thread.h"
#include "timer.h"
#include <string>
#include <math.h>
#include <stdio.h>

#ifndef WINBUILD
#include <process_controller.h>
#ifdef NUKLEAR_GLES2
#include "window_gles2.h"
#else
#include "window_glx.h"
#endif
#else
#include "window_gdi.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class ThreadTest : public Thread
{
	unsigned long time;
public:
	UserEvent event;
	ThreadTest() : Thread(true) {
		time = NkWindow::get()->timestamp();
	}

	virtual void entry() {
		if (NkWindow::get()->timestamp() - time > 1000){
			event.push(0, (void*)time);
			time = NkWindow::get()->timestamp();
		}
		printf("Thread !!\n");
#ifndef WINBUILD
		usleep(50000);
#else
		Sleep(2000);
#endif
	}
};

class test {
	NkWindow* screen;
	NkChartSlot* m_slot0;
	Timer timer;
	ThreadTest threadtest;
public:
#ifndef WINBUILD
#ifdef NUKLEAR_GLES2
		test() : screen(NkWindowGLES2::create(800,600, true)), timer(3000, false){
#else
		test() : screen(NkWindowGLX::create(800,600, true)), timer(3000, false){
#endif
#else
		test() : screen(NkWindowGDI::create(800,600, true)), timer(5000, false){
#endif
		screen->add_font("test", "/sources/GIT/nuklear/extra_font/Raleway-Bold.ttf", 16);
		screen->add_font("test2", "/sources/GIT/nuklear/extra_font/kenvector_future_thin.ttf", 24);
		screen->load_fonts();
		screen->use_font("test");

		NkButton *button = new NkButton("Button 1");
		NkButton *button1 = new NkButton("Button 2", NkButton::PLUS);
		NkCheckbox *check = new NkCheckbox("Checkbox");
		NkCheckbox *check2 = new NkCheckbox("Checkbox2");
		NkWidget *widget = new NkWidget("Test widget", 0, 0, 400, 500);
		NkLayoutRowDynamic *layout = new NkLayoutRowDynamic(30, 2, false);
		NkLayoutRow *layout2 = new NkLayoutRow(90, 100, 1);
		NkTree* tree = new NkTree;
		NkCanvasWindow* canvaswin = new NkCanvasWindow("Canvas", 150, 150, 300, 300);
		NkImage* image = new NkImage("/sources/GIT/nuklear/example/images/image9.png");
		NkCombo* combo = new NkCombo("Combo");
		NkComboImage* comboimg = new NkComboImage("ComboImg", 80);
		NkSliderFloat* slider = new NkSliderFloat("Slider", 0, 100, 50, 1);
		NkProgress* progress = new NkProgress("Progress", 100, 20, false);
		NkLabel* label = new NkLabel("Label test", TEXT_RIGHT);
		NkCanvas* canvas = new NkCanvas("Canvas");
		NkLayoutRow *layout_canvas = new NkLayoutRow(400, 400, 1);
		NkLayoutRowDynamic *layout_chart = new NkLayoutRowDynamic(400, 1);
		NkEditValue* edit_val = new NkEditValue(NkEditValue::FILTER_NONE);
		NkPropertyInt* prop_int_val = new NkPropertyInt("Property int", 0, 100, 10, 1);
		NkPropertyFloat* prop_flt_val = new NkPropertyFloat("Property flt", 0, 100, 10, 1);
		NkChart* chart = new NkChart(NkChart::CHART_LINES, 1000, -1.f, 1.f);
		m_slot0 = chart->get_slot(0);
		NkColorPicker* picker = new NkColorPicker();
		std::vector<std::string> names;
		names.push_back("Theme 1");
		names.push_back("Theme 2");
		names.push_back("Theme 3");
		names.push_back("Theme 4");
		names.push_back("Theme 5");
		names.push_back("Theme 6");
		NkCheckList* checklist = new NkCheckList(names);



		comboimg->add_element(image, "Image1");
		combo->add_element("Test1");
		combo->add_element("Test2");
		combo->add_element("Test3");

		widget->set_movable(true);
		widget->set_title_on(false);
		widget->set_closable(true);

		button->set_row_width(50);
		check->set_row_width(80);
		button1->set_row_width(60);

		layout->add_element(button);
		layout->add_element(check);
		layout->add_element(button1);
		layout->add_element(check2);

		tree->set_text_font("native");

		edit_val->set_value(2.0f);
		edit_val->set_multiline(true);
		edit_val->set_text_font("test2");
		edit_val->set_autolayout(true);
		edit_val->set_row_height(80);

		prop_int_val->set_text_font("test2");
		prop_flt_val->set_text_font("test2");

		layout_chart->add_element(chart);
		tree->add_element(checklist, "checklist", true);
		tree->add_element(picker, "picker", true);
		tree->add_element(layout_chart, "chart", false);
		tree->add_element(prop_int_val, "Property 1", true);
		tree->add_element(prop_flt_val, "Property 2", true);
		tree->add_element(layout, "Tree 1", true);
		tree->add_element(edit_val, "Floating val", true);
		layout2->add_element(image);
		tree->add_element(layout2, "Tree 2", true);
		tree->add_element(combo, "Combo strings", true);
		tree->add_element(comboimg, "Combo image", true);
		tree->add_element(slider, "Test slider", true);
		tree->add_element(progress, "Test progress", true);
		tree->add_element(label, "Test label", true);
		tree->add_element(layout_canvas, "Test canvas", true);
		layout_canvas->add_element(canvas);

		widget->add_element(tree);

		CONNECT_CALLBACK(button, on_cb_test)

		screen->add_widget(widget);

		screen->set_stye(NkWindow::THEME_DARK);

		CONNECT_CALLBACK2((checklist), on_change, this);
		CONNECT_CALLBACK((&timer), on_timer);
		timer.start();
		CONNECT_CALLBACK((&threadtest.event), on_thread);
		threadtest.start();
	}

	~test(){
		delete screen;
	}

	void run(){
		screen->run();
	}

	DECLARE_METHODS(on_cb_test)
	DECLARE_METHODS(on_timer)
	DECLARE_METHODS(on_change)
	DECLARE_METHODS(on_thread)
};

IMPLEMENT_CALLBACK_METHOD(on_cb_test, test)
{
	printf("Button !\n");
	timer.stop();
	exit(0);
}

IMPLEMENT_CALLBACK_METHOD(on_timer, test)
{
	printf("Timer !\n");
}

IMPLEMENT_CALLBACK_METHOD(on_change, test)
{
	int which = *((int*)data);
	screen->set_stye((NkWindow::StyleTheme)which);
}

IMPLEMENT_CALLBACK_METHOD(on_thread, test)
{
	printf("Thread callback\n");
}
#ifndef WINBUILD
int main(int argc, char* argv[])
#else
#ifdef WINCE
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
#else
int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
#endif
{
	test test;
	test.run();
	return 0;
}

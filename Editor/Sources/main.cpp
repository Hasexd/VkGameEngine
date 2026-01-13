#include "Application.h"
#include "Editor.h"

int main()
{
	Core::Application app("Editor", 1280, 720);
	app.PushLayer<Editor>();

	app.Run();
}
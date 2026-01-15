#include "Application.h"
#include "Editor.h"

int main()
{
	Core::Application app("Editor", 1080, 720);
	app.PushLayer<Editor>();

	app.Run();
}
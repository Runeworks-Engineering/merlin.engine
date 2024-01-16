#include "Merlin.h"
#include "ExampleLayer.h"

using namespace Merlin;

class Example : public Application
{
public:
	Example()
		: Application("OpenGL Examples")
	{
		PushLayer(new ExampleLayer());
	}
};

int main()
{
	std::unique_ptr<Example> app = std::make_unique<Example>();
	app->Run();
}
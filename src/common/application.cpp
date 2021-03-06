#include "application.hpp"

#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX

#include <bx/math.h>
#include <bgfx/platform.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <fstream>

#include "imgui_bgfx.h"

namespace app {

// application

void Application::keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	ImGuiIO& io = ImGui::GetIO();
	if ( key >= 0 && key < IM_ARRAYSIZE( io.KeysDown ) )
	{
		if ( action == GLFW_PRESS )
		{
			io.KeysDown[ key ] = true;
		}
		else if ( action == GLFW_RELEASE )
		{
			io.KeysDown[ key ] = false;
		}
	}

	io.KeyCtrl = io.KeysDown[ GLFW_KEY_LEFT_CONTROL ] || io.KeysDown[ GLFW_KEY_RIGHT_CONTROL ];
	io.KeyShift = io.KeysDown[ GLFW_KEY_LEFT_SHIFT ] || io.KeysDown[ GLFW_KEY_RIGHT_SHIFT ];
	io.KeyAlt = io.KeysDown[ GLFW_KEY_LEFT_ALT ] || io.KeysDown[ GLFW_KEY_RIGHT_ALT ];
	io.KeySuper = io.KeysDown[ GLFW_KEY_LEFT_SUPER ] || io.KeysDown[ GLFW_KEY_RIGHT_SUPER ];

	if ( !io.WantCaptureKeyboard )
	{
		  Application* app = (   Application* )glfwGetWindowUserPointer( window );
		app->onKey( key, scancode, action, mods );
	}
}

void Application::charCallback( GLFWwindow* window, unsigned int codepoint )
{
	Application* app = (Application*)glfwGetWindowUserPointer( window );
	ImGuiIO& io = ImGui::GetIO();
	io.AddInputCharacter( codepoint );
	app->onChar(codepoint);
}

void Application::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
{
	Application* app = (Application* )glfwGetWindowUserPointer( window );
	app->onCharMods( codepoint, mods );
}

void Application::mouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
	ImGuiIO& io = ImGui::GetIO();
	if ( button >= 0 && button < IM_ARRAYSIZE( io.MouseDown ) )
	{
		if ( action == GLFW_PRESS )
		{
			io.MouseDown[ button ] = true;
		}
		else if ( action == GLFW_RELEASE )
		{
			io.MouseDown[ button ] = false;
		}
	}

	if ( !io.WantCaptureMouse )
	{
		Application* app = ( Application* )glfwGetWindowUserPointer( window );
		app->onMouseButton( button, action, mods );
	}
}

void Application::cursorPosCallback( GLFWwindow* window, double xpos, double ypos )
{
	Application* app = ( Application* )glfwGetWindowUserPointer( window );
	app->onCursorPos( xpos, ypos );
}

void Application::cursorEnterCallback( GLFWwindow* window, int entered )
{
	Application* app = ( Application* )glfwGetWindowUserPointer( window );
	app->onCursorEnter( entered );
}

void Application::scrollCallback( GLFWwindow* window, double xoffset, double yoffset )
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += ( float )xoffset;
	io.MouseWheel += ( float )yoffset;
	
	if ( !io.WantCaptureMouse )
	{
		Application* app = (Application*)glfwGetWindowUserPointer( window );
		app->mMouseWheelH += ( float )xoffset;
		app->mMouseWheel += ( float )yoffset;
		app->onScroll( xoffset, yoffset );
	}
}

void Application::dropCallback( GLFWwindow* window, int count, const char** paths )
{
	Application* app = (Application*)glfwGetWindowUserPointer( window );
	app->onDrop( count, paths );
}

void Application::windowSizeCallback( GLFWwindow* window, int width, int height )
{
	Application* app = (Application*)glfwGetWindowUserPointer( window );
	app->mWidth = width;
	app->mHeight = height;
	app->reset( app->mReset );
	app->onWindowSize( width, height );
}

Application::Application( const char* title, uint32_t width, uint32_t height )
{
	mWidth = width;
	mHeight = height;
	mTitle = title;
}

int Application::run( int argc, char** argv, bgfx::RendererType::Enum type, uint16_t vendorId, uint16_t deviceId, bgfx::CallbackI* callback, bx::AllocatorI* allocator )
{
	// Initialize the glfw
	if ( !glfwInit() )
	{
		return -1;
	}

	// Create a window
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	mWindow = glfwCreateWindow( getWidth(), getHeight(), getTitle(), NULL, NULL );
	if ( !mWindow )
	{
		glfwTerminate();
		return -1;
	}

	// Setup input callbacks
	glfwSetWindowUserPointer( mWindow, this );
	glfwSetKeyCallback( mWindow, keyCallback );
	glfwSetCharCallback( mWindow, charCallback );
	glfwSetCharModsCallback( mWindow, charModsCallback );
	glfwSetMouseButtonCallback( mWindow, mouseButtonCallback );
	glfwSetCursorPosCallback( mWindow, cursorPosCallback );
	glfwSetCursorEnterCallback( mWindow, cursorEnterCallback );
	glfwSetScrollCallback( mWindow, scrollCallback );
	glfwSetDropCallback( mWindow, dropCallback );
	glfwSetWindowSizeCallback( mWindow, windowSizeCallback );

	// Setup bgfx
	bgfx::PlatformData platformData;
	memset( &platformData, 0, sizeof( platformData ) );
	platformData.nwh = ( void* )( uintptr_t )glfwGetX11Window( mWindow );
	platformData.ndt = glfwGetX11Display();
	bgfx::setPlatformData( platformData );

	// Init bgfx
	bgfx::Init init;
	init.type = type;
	init.vendorId = vendorId;
	init.deviceId = deviceId;
	init.callback = callback;
	init.allocator = allocator;
	bgfx::init( init );
	// bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

	// Setup ImGui
	ImBgfx::init(mWindow);

	// Initialize the application
	reset();
	initialize( argc, argv );

	// Loop until the user closes the window
	float lastTime = 0;
	float dt;
	float time;
	while ( !glfwWindowShouldClose( mWindow ) )
	{
		time = ( float )glfwGetTime();
		dt = time - lastTime;
		lastTime = time;

		glfwPollEvents();
		ImBgfx::events( dt );
		ImGui::NewFrame();
		update( dt );
		ImGui::Render();
		ImBgfx::render( ImGui::GetDrawData() );
		bgfx::frame();
	}

	// Shutdown application and glfw
	int ret = shutdown();
	ImBgfx::shutdown();
	bgfx::shutdown();
	glfwTerminate();
	return ret;
}

void Application::reset( uint32_t flags )
{
	mReset = flags;
	bgfx::reset( mWidth, mHeight, mReset );
	ImBgfx::reset(uint16_t(getWidth()), uint16_t(getHeight()));
	onReset();
}

uint32_t Application::getWidth() const
{
	return mWidth;
}

uint32_t Application::getHeight() const
{
	return mHeight;
}

void Application::setSize( int width, int height )
{
	glfwSetWindowSize( mWindow, width, height );
}

const char* Application::getTitle() const
{
	return mTitle;
}

void Application::setTitle( const char* title )
{
	mTitle = title;
	glfwSetWindowTitle( mWindow, title );
}

bool Application::isKeyDown( int key ) const
{
	ImGuiIO& io = ImGui::GetIO();
	if ( key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST || io.WantCaptureKeyboard )
	{
		return false;
	}

	return glfwGetKey( mWindow, key ) == GLFW_PRESS;
}

bool Application::isMouseButtonDown( int button ) const
{
	ImGuiIO& io = ImGui::GetIO();
	if ( button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST || io.WantCaptureMouse )
	{
		return false;
	}
	
	return glfwGetMouseButton( mWindow, button ) == GLFW_PRESS;
}

float Application::getMouseWheelH() const
{
	return mMouseWheelH;
}

float Application::getMouseWheel() const
{
	return mMouseWheel;
}

}

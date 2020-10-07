#include "pch.h"
#include "HelloTriangleApplication.h"
#include "Application.h"

int main()
{
    {
        auto app = scatter::VulkanApplication(1920, 1080);
    
        try
        {
            app.update(10.0f);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }

    system("pause");

    return EXIT_SUCCESS;
}
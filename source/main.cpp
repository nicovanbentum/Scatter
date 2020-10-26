#include "pch.h"
#include "HelloTriangleApplication.h"
#include "Application.h"

int main() {
    {
        HelloTriangleApplication myApp;
        //myApp.wakeUpBool = false;
        if (myApp.wakeUpBool)
        {
            std::cout << "Triangle Active \n";
            try {
                myApp.run();
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::cout << "Scatter active \n";
            auto app = scatter::VulkanApplication();
            app.init(1920, 1080);

            try {
                app.update(10.0f);
            }
            catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }

            app.destroy();
        }

    }
    system("pause");

    return EXIT_SUCCESS;
}
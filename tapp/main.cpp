/***************
CTAG TBD >>to be determined<< is an open source eurorack synthesizer module.

A project conceived within the Creative Technologies Arbeitsgruppe of
Kiel University of Applied Sciences: https://www.creative-technologies.de

(c) 2020 by Robert Manzke. All rights reserved.

The CTAG TBD software is licensed under the GNU General Public License
(GPL 3.0), available here: https://www.gnu.org/licenses/gpl-3.0.txt

The CTAG TBD hardware design is released under the Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0).
Details here: https://creativecommons.org/licenses/by-nc-sa/4.0/

CTAG TBD is provided "as is" without any express or implied warranties.

License and copyright details for specific submodules are included in their
respective component folders / files if different from this license.
***************/

//  #define GLEW_STATIC true  // MB 20210612 ??? https://coderedirect.com/questions/448523/receiving-undefined-references-to-various-windows-libraries-when-compiling-with
#include <boost/program_options.hpp>
#include <boost/dll.hpp>
#include <libserialport.h>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <cstdlib>
#include <chrono>
#include <thread>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else

#include <SDL_opengl.h>

#endif

#include "WebServer.hpp"

using namespace std;
namespace po = boost::program_options;

#ifndef SP_PRIV
#define SP_PRIV
#endif
#define IOCTL_USB_GET_ROOT_HUB_NAME 0x220408            // MB 20211208 Windows USB Device constant
#define IOCTL_USB_GET_NODE_CONNECTION_NAME 0x220414     // MB 20211208 Windows USB Device constant

#include <ImGuiFileBrowser.h>

#include <string>
#include <vector>


#include <stdio.h>

#ifdef _WIN32
#define BOOST_USE_WINDOWS_H 1
// ### #include <windows.h>
#include <process.h>
#include <shellapi.h>
#endif

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <boost/process/child.hpp>

using namespace boost::process;
namespace bp = ::boost::process;
using namespace boost::asio;

imgui_addons::ImGuiFileBrowser file_dialog; // https://github.com/gallickgunner/ImGui-Addons

bool ok_to_overwrite_dialogue() {
    bool result = false;
    {
        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("Overwrite?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("OK will overwrite current data of your TBD\nClick 'Cancel' to avoid this!\n\n");
            ImGui::Separator();
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImGui::PopStyleVar();
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                result = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                result = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    return result;
}

// ### int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow)
int main(int ac, char **av) {
    // parse command line args
    string serialPort = "COM3";
    unsigned short iPort = 3030;
    bool noGui = false;
    po::options_description desc(string(av[0]) + " options");
    po::variables_map vm;

    auto base_path {boost::dll::program_location().parent_path()};
    boost::filesystem::current_path(base_path);

    static const char *local_ports[] = {"1010", "2020", "3030", "4040", "6060"};
    static const char *current_port = "3030";
#if __WIN64__
    static char sample_package[4096] = "./flashtbd/sample-rom.tbd";
#elif __APPLE__
    auto sample_rom_path = base_path;
    sample_rom_path.append("/bin/sample-rom.tbd");
    static char sample_package[4096];
    strcpy(sample_package, sample_rom_path.c_str());
#endif

    // --- Boost-Library "process" related stuff ---
    ipstream pipe_stream;
    bool start_tapp = false;
    std::vector <std::string> flash_output_list;
    std::string line;
    child *tapp_child = (child *) NULL;
    bool tapp_running = false;
    bool erase_running = false;
    bool select_sample_pack = false;
    // --- IO stuff ---
    char txt_buf[128];
    static char cmd_buf[4096];

    // --- Parameters for tapp ---
    WebServer *webServerp = NULL;

    try {
        desc.add_options()
                ("help,h", "this help message")
                ("port,p", po::value<unsigned short>(&iPort)->default_value(3030), "http port, default 3030")
                // ("ser,s", po::value<string>(&serialPort)->required(), "serial port, e.g. /dev/ttyUSB0")  MB 20211214 allow default, especially in GUI-mode!
                ("ser,s", po::value<string>(&serialPort)->default_value("/dev/ttyUSB0"),
                 "serial port, e.g. /dev/ttyUSB0")
                ("nogui,n", po::bool_switch(&noGui), "start in console mode, no gui");

        po::store(po::parse_command_line(ac, av, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << desc << "\n";
            return 1;
        }
    } catch (const boost::program_options::required_option &e) {
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 1;
        } else {
            throw e;
        }
    }

    if (noGui) {
        char input;

        webServerp = new WebServer;
        webServerp->Start(iPort, serialPort);
        std::cin.get(input);
    } else {
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // Setup SDL
        // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
        // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
            printf("Error: %s\n", SDL_GetError());
            return -1;
        }

        // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
        // GL ES 2.0 + GLSL 100
        const char* glsl_version = "#version 100";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
        // GL 3.2 Core + GLSL 150
        const char* glsl_version = "#version 150";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
        // GL 3.0 + GLSL 130
        const char *glsl_version = "#version 130";
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

        // Create window with graphics context
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(
                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
        SDL_Window *window = SDL_CreateWindow("Terminal Application for TBD", SDL_WINDOWPOS_CENTERED,
                                              SDL_WINDOWPOS_CENTERED, 600, 400, window_flags);
        SDL_GLContext gl_context = SDL_GL_CreateContext(window);
        SDL_GL_MakeCurrent(window, gl_context);
        SDL_GL_SetSwapInterval(1); // Enable vsync

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // Setup Platform/Renderer backends
        ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
        ImGui_ImplOpenGL3_Init(glsl_version);

        // Load Fonts
        // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
        // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
        // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
        // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
        // - Read 'docs/FONTS.md' for more instructions and details.
        // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
        //io.Fonts->AddFontDefault();
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
        //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
        //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
        //IM_ASSERT(font != NULL);

        // Main loop
        bool done = false;
        while (!done) {
            io.DeltaTime = 1.f / 30.f; // frame rate
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (event.type == SDL_QUIT)
                    done = true;
                if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                    event.window.windowID == SDL_GetWindowID(window))
                    done = true;
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplSDL2_NewFrame();
            ImGui::NewFrame();


            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
            {
                static float f = 0.0f;
                static int counter = 0;

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

                ImGui::Begin(
                        "Terminal-Application and Flashing of Firmware/Samples for the TBD");                           // Create a window called "Hello, world!" and append into it.
                /* A pointer to a null-terminated array of pointers to                * struct sp_port, which will contain the ports found.*/

                // --- Optional [file] dialogs to choose sample-package-location_and_name or listener-port
                if (!tapp_child && !erase_running && !tapp_running) // No action has been triggered yet!
                {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Current Package for sample-upload: '%s'", sample_package);
                    sprintf(txt_buf, "Click to select different Sample-Package (please use short path+filename)");
                    if (ImGui::Button(
                            txt_buf))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                        select_sample_pack = true;

                    if (select_sample_pack)    // We have to use a bool variable to fulfill Dear ImGui's needs of its ID-Stack!
                    {
                        ImGui::OpenPopup(
                                "Select Sample-Pack - long path+filename may crash the transport-tool!"); // String for showFileDialog must be identical!
                        if (file_dialog.showFileDialog(
                                "Select Sample-Pack - long path+filename may crash the transport-tool!",
                                imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(700, 310), ".tbd")) {
                            select_sample_pack = false;
                            std::cout << "file-dialog end" << std::endl;
                            std::cout << "./flashtbd/" + file_dialog.selected_fn
                                      << std::endl;      // The name of the selected file or directory in case of Select Directory dialog mode
                            std::cout << file_dialog.selected_path << std::endl;
                            std::cout << file_dialog.selected_path
                                      << std::endl;     // Path already includes Filename (at least on Windows)!
                            strncpy(sample_package, file_dialog.selected_path.c_str(), sizeof(sample_package) - 1);
                        }
                        if (file_dialog.cancelled())
                            select_sample_pack = false;
                    }
                    // --- Select port for listener of tapp --- // Inspired by: https://github.com/ocornut/imgui/issues/1658
                    ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                       "Click below to select different port for Terminal-Adapter and Browser",
                                       sample_package);
                    bool is_selected = false;
                    if (ImGui::BeginCombo("##combo",
                                          current_port)) // The second parameter is the label previewed before opening the combo.
                    {
                        for (int i = 0; i < IM_ARRAYSIZE(local_ports); i++) {
                            is_selected = (current_port ==
                                           local_ports[i]); // You can store your selection however you want, outside or inside your objects
                            if (ImGui::Selectable(local_ports[i], is_selected))
                                current_port = local_ports[i];
                            if (is_selected)
                                ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                        }
                        std::cerr << current_port << std::endl;
                        ImGui::EndCombo();
                    }
                }
                struct sp_port **port_list;
                int result = sp_list_ports(&port_list);
                if (result == SP_OK && port_list[0]) {
                    if (!tapp_child && !tapp_running && !erase_running) // Before executing an action?
                    {
                        ImGui::Text(
                                "Click 'FLASH/ERASE TBD [...(Port)]' to reset or send new firmware/samples on given device+port");               // Display some text (you can use a format strings too)
                        ImGui::Text(
                                "Click 'TAPP TBD [...(Port)]' to start terminal-adapter for Browser on given device+port");               // Display some text (you can use a format strings too)
                        ImGui::Text(
                                "If in doubt what to choose: Normally TBD will be connected to: 'USB-Serial CH340'\n");               // Display some text (you can use a format strings too)
                    }
                    for (int i = 0; port_list[i] != NULL; i++) {
                        struct sp_port *port = port_list[i];   // Get the name of the port.
                        char *port_name = sp_get_port_name(port);
                        string pname {port_name};
#ifdef __APPLE__
                        // does port name include usbserial? if not-> don't list!
                        if(pname.find("usbserial") == std::string::npos) continue;
#endif

                        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Port available: %s", port_name);
                        char *description = sp_get_port_description(port);
                        int transport_type = sp_get_port_transport(port);
                        //std::cerr << "Transport type " << transport_type << std::endl;
                        if (1){//transport_type == SP_TRANSPORT_USB || SP_TRANSPORT_NATIVE) {
                            if ((tapp_child && !tapp_child->running()) || erase_running) {
                                if (!erase_running) {
                                    ImGui::TextColored(ImVec4(1, 0, 0, 1),
                                                       "Current action finished (or currently not possible) \nclose tapp-tool now or start again to trigger a new action");
                                    ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                                       "Please note, that only one tapp-tool can access the TBD, \nso make sure only one is running!");
                                } else {
                                    ImGui::TextColored(ImVec4(1, 0, 0, 1),
                                                       "Erase is running, this is slow! Check background window for status...");
                                    ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                                       "Please close all windows as soon as the erase-process has finished!");
                                }
                            } else {
                                static int flash_mode = 0;   // Caution: needs to be static becaus of Dear ImGUI's framelogic: Decides if/which flashing mode we selected and enabled overwrite via popup-dialogue...
                                if (!tapp_child && !tapp_running) // No action triggered yet
                                {
                                    sprintf(txt_buf, "ERASE TBD [%s]", description);
                                    if (ImGui::Button(
                                            txt_buf))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                                    {
                                        flash_mode = 1;
                                        ImGui::OpenPopup("Overwrite?");
                                    }
                                    if (flash_mode == 1) {
                                        if (ok_to_overwrite_dialogue())    // Contains modal "Overwrite?" popup-dialogue!
                                        {

                                            erase_running = true;
#ifdef __WIN64__
                                            sprintf(cmd_buf, ".\\flashtbd\\esptool_erasetbd.bat %s", port_name);
                                            ShellExecute(0, "open", ".\\flashtbd\\esptool_erasetbd.bat", port_name, 0, SW_SHOWDEFAULT);
#elif __APPLE__
                                            sprintf(cmd_buf, "bin/esptool/esptoolmac -p %s -b 460800 erase_flash", port_name);
                                            auto c = child((char*)cmd_buf, std_out > pipe_stream);
                                            std::cout << "Erasing..." << std::endl;
                                            c.wait();
                                            std::cout << "Done, restart tapp!" << std::endl;
                                            //tapp_child = new child((char *) cmd_buf, std_out > pipe_stream);
#else
#endif
                                        }
                                    }
                                    sprintf(txt_buf, "FLASH Firmware TBD [%s]", description);
                                    if (ImGui::Button(
                                            txt_buf))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                                    {
                                        flash_mode = 2;
                                        ImGui::OpenPopup("Overwrite?");
                                    }
                                    if (flash_mode == 2) {
                                        if (ok_to_overwrite_dialogue())    // Contains modal "Overwrite?" popup-dialogue!
                                        {
#ifdef __WIN64__
                                            sprintf(cmd_buf, ".\\flashtbd\\esptool_flashtbd.bat %s", port_name);
#elif __APPLE__
                                            sprintf(cmd_buf, "bin/esptool/esptoolmac -p %s -b 460800 --before default_reset --after hard_reset --chip esp32 write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x8000 bin/partition-table.bin 0xd000 bin/ota_data_initial.bin 0x1000 bin/bootloader.bin 0x10000 bin/ctag-tbd.bin 0x610000 bin/storage.bin", port_name);
#else
#endif
                                            tapp_child = new child((char *) cmd_buf, std_out > pipe_stream);
                                        }
                                    }
                                    sprintf(txt_buf, "FLASH SampleROM TBD [%s] Samples: %s", description,
                                            sample_package);
                                    if (ImGui::Button(
                                            txt_buf))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                                    {
                                        flash_mode = 3;
                                        ImGui::OpenPopup("Overwrite?");
                                    }
                                    if (flash_mode == 3) {
                                        if (ok_to_overwrite_dialogue())    // Contains modal "Overwrite?" popup-dialogue!
                                        {
#ifdef __WIN64__
                                            if (boost::filesystem::exists(
                                                    "./flashtbd/usersamples.tmp"))    // no copy_file() "ignore overwrite", so we delete first - using the extension *.tmp prevents us from having users select previous versions and thus get copy errors after delete ;-)
                                                boost::filesystem::remove("./flashtbd/usersamples.tmp");
                                            boost::filesystem::copy_file(sample_package, "./flashtbd/usersamples.tmp");
                                            sprintf(cmd_buf, ".\\flashtbd\\esptool_flashsample.bat %s %s", port_name,
                                                    ".\\flashtbd\\usersamples.tmp"); // ### sample_package);
#elif __APPLE__
                                            sprintf(cmd_buf, "bin/esptool/esptoolmac -p %s -b 460800 write_flash --flash_mode dio -fs detect 0xB00000 %s", port_name, sample_package);
#else
#endif
                                            tapp_child = new child((char *) cmd_buf, std_out > pipe_stream);
                                        }
                                    }
                                    sprintf(txt_buf, "TAPP TBD [%s]", description);
                                    if (ImGui::Button(
                                            txt_buf))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                                    {
                                        webServerp = new WebServer;
                                        iPort = atoi(current_port);
                                        webServerp->Start(iPort, port_name);
                                        static char http_buf[BUFSIZ];

#ifdef __APPLE__
                                        sprintf(http_buf, "open http://localhost:%s/ &", current_port);
                                        std::system(http_buf);
#else
                                        sprintf(http_buf, "open http://localhost:%s/", current_port);
                                        mShellExecute(0, "open", http_buf, cmd_buf, 0, SW_SHOWDEFAULT);
#endif
                                        tapp_running = true;
                                    }
                                } else  // Terminal application running?
                                {
                                    if (tapp_running) {
                                        ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                                           "TAPP is running, if browser did not start yet use 'localhost:%s' in your browser!",
                                                           current_port);
                                        ImGui::TextColored(ImVec4(1, 1, 0, 1),
                                                           "On error close tapp-console window or this app, check cables and try again");
                                    }
                                }
                            }
                        }
                    }
                    if (tapp_child && tapp_child->running() && pipe_stream && std::getline(pipe_stream, line) &&
                        !line.empty()) {
                        std::cerr << line << std::endl;
                        flash_output_list.push_back(
                                line);  // add to vector of strings, so that we can display it in the scroling window below via Dear ImGui
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Process output (action running)");
                    } else {
                        if (tapp_child && !tapp_child->running())
                            ImGui::TextColored(ImVec4(0.68f, 0.85f, 0.9f, 1),
                                               "Process output (action terminated)");  // RGB: light-blue
                    }
                    ImGui::BeginChild("Scrolling");
                    for (vector<string>::iterator t = flash_output_list.begin(); t != flash_output_list.end(); ++t)
                        ImGui::Text(t->c_str());
                    ImGui::SetScrollHereY(
                            1.0f);      // Scroll to bottom => latest entries always visible! - bug?: currently only scrolls to second-last line
                    ImGui::EndChild();
                } else    // No serial ports available, put out warning
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1),
                                       "\nNo serial ports found, check cables and if TBD is on!");     // RGB, alpha
                }
                ImGui::End();
            }
            // Rendering
            ImGui::Render();
            glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
            glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w,
                         clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            SDL_GL_SwapWindow(window);
            // limit frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_GL_DeleteContext(gl_context);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    // --- we add the gui via Dear ImGui now instead --- ### webServer.Stop();

    return 0;
}

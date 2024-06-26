#ifndef SLIC3R_HPP
#define SLIC3R_HPP

#include "libslic3r/Config.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r
{
    namespace IO
    {
        enum ExportFormat : int
        {
            AMF,
            OBJ,
            STL,
            TMF,
            Gcode
        };
    }

    class CLI
    {
        public:
            /// @brief TODO: Add documentation
            /// @param argc 
            /// @param argv 
            /// @return 
            int run(int argc, char **argv);

        private:
            /// @brief TODO: Add documentation
            /// @param argc 
            /// @param argv 
            /// @return 
            bool setup(int argc, char **argv);

            /// @brief Prints usage of the CLI.
            /// @param include_print_options 
            /// @param printer_technology 
            void print_help(bool include_print_options = false, PrinterTechnology printer_technology = ptAny) const;

            /// @brief Exports loaded models to a file of the specified format, according to the options affecting
            /// output filename.
            /// @param format 
            /// @return 
            bool export_models(IO::ExportFormat format);

            /// @brief TODO: Add documentation
            /// @return 
            bool has_print_action() const
            {
                return (m_config.opt_bool("export_gcode") || m_config.opt_bool("export_sla"));
            }

            /// @brief TODO: Add documentation
            /// @return 
            bool processed_profiles_sharing();

            /// @brief TODO: Add documentation
            /// @param printer_technology 
            /// @return 
            bool check_and_load_input_profiles(PrinterTechnology &printer_technology);

            /// @brief TODO: Add documentation
            /// @param model 
            /// @param format 
            /// @return 
            std::string output_filepath(const Model &model, IO::ExportFormat format) const;

            DynamicPrintAndCLIConfig m_config;
            DynamicPrintConfig m_print_config;
            DynamicPrintConfig m_extra_config;

            std::vector<std::string> m_input_files;
            std::vector<std::string> m_actions;
            std::vector<std::string> m_transforms;
            std::vector<std::string> m_profiles_sharing;

            std::vector<Model> m_models;
    };

} // namespace Slic3r

#endif

#include "Patch.hpp"

namespace fatigue {
    // Setup

    void Patch::init()
    {
        if (!m_region.isValid()) {
            logWarning("Patch region is invalid");
            return;
        }

        // If given a pattern, find the address
        if (!m_pattern.empty()) {
            find();
        }

        if (!m_found) {
            // Warning given in find()
            // logWarning("Patch failed to find address");
            return;
        }

        // Backup matched data and original data
        backup();

        // Quick consistency check
        if (m_matched.size() != patternSize()) {
            logWarning(std::format(
                "Patch matched data size ({}) does not match pattern size ({})",
                m_matched.size(), patternSize()
            ));
        }

        if (m_original.size() != m_patch.size()) {
            logWarning(std::format(
                "Patch original data size ({}) does not match patch size ({})",
                m_original.size(), m_patch.size()
            ));
        }

        if (m_address + offset() < 0) {
            logWarning(std::format(
                "Patch address {:#x} + offset {} is negative (this is probably a mistake)",
                m_address, offset()
            ));
        }
    }

    void Patch::find()
    {
        m_address = 0;
        m_found = false;

        if (m_region.isValid()) {
            m_matches = m_region.find(m_pattern);
            m_found = !m_matches.empty();
            if (m_found) {
                m_address = m_matches.front();
            } else {
                logWarning(std::format(
                    "Patch failed to find pattern:\n"
                    "  Region: {}\n"
                    "  Pattern: {}",
                    m_region.toString(), m_pattern
                ));
            }

            if (m_matches.size() > 1) {
                std::string show = "";
                for (size_t i = 1; i < defaultShowMatches && i < m_matches.size(); i++) {
                    show += std::format("{:#x}, ", m_matches.at(i));
                }

                logWarning(std::format(
                    "Patch found {} matches for pattern (pattern may be too loose):\n"
                    "  Region: {}\n"
                    "  Pattern: {}\n"
                    "  Using first match at {:#x}\n"
                    "  Also matched at {}{}",
                    m_matches.size(), m_region.toString(), m_pattern, m_address,
                    show, (m_matches.size() > defaultShowMatches ? "..." : "")
                ));
            }
        } else {
            logWarning(std::format(
                "Patch region is invalid:\n"
                "  Region: {}\n"
                "  Pattern: {}",
                m_region.toString(), m_pattern
            ));
        }
    }

    void Patch::backup()
    {
        if (!isValid()) return;

        m_matched.clear();
        m_original.clear();

        if (!m_pattern.empty()) {
            m_matched.resize(patternSize());
            m_region.read(m_address, m_matched.data(), patternSize());
        }

        m_original.resize(m_patch.size());
        m_region.read(patchAddress(), m_original.data(), m_patch.size());
    }

    // Application

    bool Patch::apply()
    {
        if (!isValid()) {
            logWarning("Cannot apply, patch is invalid");
            return false;
        }

        // If the patch data is empty, we can't apply the patch
        if (m_patch.empty()) {
            logWarning("Cannot apply, patch data is empty");
            return false;
        }

        // If the original data is empty, we can't restore the patch, but we can still apply it
        if (m_original.empty()) {
            logWarning("Original data is empty; patch cannot be restored");
        }

        // If the patch has already been applied, just return
        if (m_applied) return true;

        // Apply the patch
        if (m_region.write(patchAddress(), m_patch.data(), m_patch.size()) != m_patch.size()) {
            logWarning("Failed to write patch data");
            return false;
        }

        m_applied = true;
        return true;
    }

    bool Patch::restore()
    {
        if (!isValid()) {
            logWarning("Cannot restore, patch is invalid");
            return false;
        }

        // If the original data is empty, we can't restore the patch
        if (m_original.empty()) {
            logWarning("Cannot restore, original data is empty");
            return false;
        }

        // If the patch has not been applied, just return
        if (!m_applied) return true;

        // Restore the patch
        if (m_region.write(patchAddress(), m_original.data(), m_original.size()) != m_original.size()) {
            logWarning("Failed to write original data");
            return false;
        }

        m_applied = false;
        return true;
    }

    bool Patch::toggle()
    {
        return m_applied ? restore() : apply();
    }

    // Utility

    std::string Patch::toString() const
    {
        std::stringstream out;
        out << "Patch on region " << m_region.toString() << std::endl;
        if (isValid()) {
            out << std::format(
                    "{} bytes at {:#x} + {} ({})",
                    m_patch.size(), patchAddress(), offset(), (m_applied ? "active" : "inactive"))
                << std::endl;

        } else {
            out << "Invalid: "
                << (m_region.isValid() ? "Patch address not found" : "Region is invalid")
                << std::endl;
        }
        return out.str();
    }

    std::string Patch::dump() const
    {
        std::stringstream out;

        // Header: Region
        out << "Patch on region " << m_region.toString() << std::endl;

        // If not valid, print why
        if (!isValid()) {
            out << Color::Bold << "Patch is invalid: " << Color::Reset
                << (m_region.isValid() ? "Patch address not found" : "Region is invalid")
                << " ❌"
                << std::endl;
        }

        // If pattern, print pattern or matched data with <> around wildcard bytes
        out << dumpPattern();

        // Original data and patch data with [] around changed bytes
        out << std::endl
            << dumpPatch();

        return out.str();
    }

    std::string Patch::dumpPattern(size_t showMatches) const
    {
        if (showMatches == 0) showMatches = defaultShowMatches;

        std::stringstream out;

        if (!m_pattern.empty()) {
            out << Color::Bold << std::format("{:{}s}", "Pattern: ", labelWidth) << Color::Reset;
            if (m_matched.empty()) {
                out << Color::Red << "Not found ❌" << Color::Reset;
            } else if (m_matches.size() > 1) {
                out << Color::Yellow << "Multiple matches ✔ " << Color::Reset << std::endl
                    << std::format("{:{}s}", "", labelWidth)
                    << "Check Patch.matches(); pattern may be too loose" << std::endl
                    << std::format("{:{}s}", "", labelWidth)
                    << std::format("Using first match at {:#x}", m_address);
            } else {
                out << Color::BrightBlue << "Found ✔" << Color::Reset;
            }
            out << std::endl;

            // Print address(es)
            if (m_matched.empty()) {
                out << dumpPatternAt(-1);
            } else {
                out << dumpPatternAt(m_address, Color::BrightBlue);

                // If multiple matches, print the first few
                if (m_matches.size() > 1) {
                    for (size_t i = 1; i < showMatches && i < m_matches.size(); i++) {
                        out << dumpPatternAt(m_matches.at(i), Color::Yellow);
                    }
                    if (m_matches.size() > showMatches) {
                        out << std::format("{:{}s}", "", labelWidth)
                            << Color::Dim << std::format("... {} more", m_matches.size() - showMatches)
                            << Color::Reset << std::endl;
                    }
                }
            }
        }

        return out.str();
    }

    std::string Patch::dumpPatternAt(long address, Color highlight) const
    {
        std::stringstream out;

        // Label
        if (address >= 0) {
            out << Color::Dim
                << std::format("{:{}s}", std::format("{:#x}: ", address), labelWidth)
                << Color::Reset;
        } else {
            // If address is -1, use not-found formatting
            out << std::format("{:{}s}", "", labelWidth);
        }

        // For each byte in the pattern, print the byte or the matched byte in <>
        std::vector<std::string> splitPattern = hex::split(m_pattern);
        // Read the matched data at the address if it's valid
        // So we can show the actual byte instead of a wildcard
        std::vector<uint8_t> matched(splitPattern.size());
        if (address >= 0) {
            m_region.read(address, matched.data(), matched.size());
        }

        for (std::size_t i = 0; i < splitPattern.size(); ++i) {
            std::string byte = splitPattern.at(i);
            // Substitute the matched byte for wildcards
            if (byte == "??") {
                if (address >= 0) {
                    out << highlight << "<"
                        << hex::toHex(&matched.at(i), sizeof(uint8_t))
                        << ">" << Color::Reset;
                } else {
                    // If address is -1, use not-found formatting
                    out << Color::Red << "<\?\?>" << Color::Reset;
                }
            } else {
                out << byte;
            }
            out << " ";
        }
        out << std::endl;

        return out.str();
    }

    std::string Patch::dumpPatch() const
    {
        std::stringstream out;

        if (m_found && !m_patch.empty()) {
            // Header
            out << Color::Bold << std::format("{:{}s}", "Patch: ", labelWidth) << Color::Reset
                << m_patch.size() << " bytes at "
                << std::format("{:#x}", m_address);

            if (offset() > 0)
                out << " + " << std::format(
                    "{:#x} ({}offset)",
                    offset(), (m_offset_fn ? "dynamic " : ""));

            out << std::endl;

            // Status
            out << Color::Bold << std::format("{:{}s}", "Status: ", labelWidth) << Color::Reset;
            if (m_applied) {
                out << Color::Green << "Active 🔨" << Color::Reset;
            } else {
                out << Color::Yellow << "Inactive" << Color::Reset;
            }
            out << std::endl;

            // Determine the context to use to show the patch
            // Size of the context should be a multiple of 16, and at least the size of the patch
            // (patch size is an int, so division will floor the result, add 1 to for the padding)
            size_t contextSize = ((m_patch.size() / 16) + 1) * 16;
            size_t contextPadding = (contextSize - m_patch.size()) / 2;
            uintptr_t contextAddress = patchAddress() - contextPadding;

            // Read the context
            std::vector<uint8_t> context(contextSize);
            m_region.read(contextAddress, context.data(), context.size());

            // Show the context with patch bytes in []
            for (size_t i = 0; i < context.size();) {
                out << Color::Dim
                    << std::format("{:{}s}", std::format("{:#x}: ", contextAddress + i), labelWidth)
                    << Color::Reset;

                for (std::size_t j = 0; j < 16; j++, i++) {
                    bool isPatch = (contextAddress + i) >= patchAddress() && (contextAddress + i) < patchAddress() + m_patch.size();

                    if (isPatch) out << (m_applied ? Color::Green : Color::Yellow) << "[";
                    out << hex::toHex(&context.at(i), sizeof(uint8_t));
                    if (isPatch) out << "]" << Color::Reset;
                    out << " ";
                }

                out << std::endl;
            }

            // Show original data and patch data, at the correct column
            out << Color::Bold << std::format("{:{}s}", "Original:", labelWidth + contextPadding * 3) << Color::Reset;
            for (uint8_t byte : m_original) {
                out << Color::Yellow << "[" << hex::toHex(&byte, sizeof(uint8_t)) << "] " << Color::Reset;
            }
            out << std::endl;

            out << Color::Bold << std::format("{:{}s}", "Patch:", labelWidth + contextPadding * 3) << Color::Reset;
            for (uint8_t byte : m_patch) {
                out << Color::Green << "[" << hex::toHex(&byte, sizeof(uint8_t)) << "] " << Color::Reset;
            }
            out << std::endl;
        } else {
            out << Color::Bold << std::format("{:{}s}", "Status: ", labelWidth) << Color::Reset
                << Color::Red << (m_patch.empty() ? "No patch data" : "No patch address") << " ❌" << Color::Reset
                << std::endl;
        }

        return out.str();
    }
}
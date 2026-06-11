#include "main.h"

class Subnet {
public:
    Subnet(uint128_t network, uint32_t prefix_length, bool is_ipv6) : prefix_length_(prefix_length), is_ipv6_(is_ipv6) {
        if(prefix_length_ > 128) throw std::invalid_argument("Invalid prefix length");

        // Apply network mask to network address
        network_ = (network & prefixMask(prefix_length_));
    }
    Subnet(const Subnet& other) : network_(other.network_), prefix_length_(other.prefix_length_), is_ipv6_(other.is_ipv6_) {}

    uint128_t getNetwork() const { return network_; }
    uint32_t getPrefixLength() const { return prefix_length_; }
    bool isIPv6() const { return is_ipv6_; }

    static uint128_t reverseByteOrder(uint128_t value) {
        uint128_t result = 0;
        for (int i = 0; i < 16; ++i) {
            uint8_t byte = (value >> (i * 8)) & 0xFF;
            result |= static_cast<uint128_t>(byte) << ((15 - i) * 8);
        }
        return result;
    }

    static uint128_t prefixMask(uint32_t prefix_length) {
        if(prefix_length > 128) throw std::invalid_argument("Invalid prefix length");
        return (prefix_length<1)?static_cast<uint128_t>(0):((~static_cast<uint128_t>(0)) << (128 - prefix_length));
    }

    bool equals(const Subnet& other) const {
        return (network_ == other.getNetwork()) && (prefix_length_ == other.getPrefixLength()) && (is_ipv6_ == other.isIPv6());
    }

    bool contains(const Subnet& other) const {
        if(is_ipv6_ != other.isIPv6()) return false;
        uint128_t subnet_mask = prefixMask(prefix_length_);
        return ((other.getNetwork() & subnet_mask) == network_ && prefix_length_ <= other.getPrefixLength());
    }

    static bool sortPrintNicely(const Subnet& first, const Subnet& second) {
        if(first.getNetwork() < second.getNetwork()) {
            return true;
        } else
        if(first.getNetwork() > second.getNetwork()) {
            return false;
        } else
        if(first.getPrefixLength() < second.getPrefixLength()) {
            return true;
        } else
        if(first.getPrefixLength() > second.getPrefixLength()) {
            return false;
        } else
            return false;
    }

    static bool sortDescendingPrecedence(const Subnet& first, const Subnet& second) {
        if(first.getPrefixLength() < second.getPrefixLength()) {
            return true;
        } else
        if(first.getPrefixLength() > second.getPrefixLength()) {
            return false;
        } else
        if(first.getNetwork() < second.getNetwork()) {
            return true;
        } else
        if(first.getNetwork() > second.getNetwork()) {
            return false;
        } else
            return false;
    }

    std::pair<Subnet, Subnet> split() const {
        if(prefix_length_ >= 128) throw std::logic_error("Unable to split host subnet");

        // Calculate the new prefix length for the smaller subnets
        uint32_t new_prefix_length = prefix_length_ + 1;

        // Calculate the network addresses of the new subnets
        uint128_t subnet1_network = network_;
        uint128_t subnet2_network = network_ | (static_cast<uint128_t>(1) << (128 - new_prefix_length));

        // Create and return the new subnets
        return std::make_pair(Subnet(subnet1_network, new_prefix_length, is_ipv6_), Subnet(subnet2_network, new_prefix_length, is_ipv6_));
    }

    static bool canMerge(const Subnet& first, const Subnet& second) {
        if(first.isIPv6() != second.isIPv6()) return false;
        if(first.getPrefixLength() != second.getPrefixLength()) return false;
        if(first.getPrefixLength() == 0) return false;
        if(first.getNetwork() == second.getNetwork()) return false;

        uint128_t new_subnet_mask = prefixMask(first.getPrefixLength()-1);
        if( (first.getNetwork() & new_subnet_mask) == (second.getNetwork() & new_subnet_mask) ) return true;
        return false;
    }

    static Subnet merge(const Subnet& first, const Subnet& second) {
        if(!canMerge(first, second)) throw std::logic_error("Unable to merge non-mergeable subnets");
        return Subnet(first.getNetwork(), (first.getPrefixLength() - 1), first.isIPv6());
    }

    static Subnet parseCidr(const std::string& subnet_cidr) {
        try {
            // Find the position of the '/' character to split the string
            size_t slash_pos = subnet_cidr.find('/');
            if(slash_pos == std::string::npos || slash_pos == 0 || slash_pos == subnet_cidr.length()-1 || subnet_cidr.find('/', slash_pos+1) != std::string::npos)
                throw std::invalid_argument("Invalid CIDR delimiter");

            // Extract the network address part of the string
            std::string network_str = subnet_cidr.substr(0, slash_pos);

            // Extract the prefix length part of the string
            std::string prefix_length_str = subnet_cidr.substr(slash_pos + 1);

            // Convert the network address from string to binary representation
            uint128_t network_address = 0;
            uint32_t prefix_length = 0;
            bool is_ipv6 = false;
            if (inet_pton(AF_INET6, network_str.c_str(), &network_address) == 1) {
                // IPv6 case
                is_ipv6 = true;
                size_t parsed_chars = 0;
                int parsed_prefix_length = std::stoi(prefix_length_str, &parsed_chars);
                if(parsed_chars != prefix_length_str.length()) throw std::invalid_argument("Invalid IPv6 prefix length");
                if(parsed_prefix_length < 0 || parsed_prefix_length > 128) throw std::invalid_argument("Invalid IPv6 prefix length");
                prefix_length = static_cast<uint32_t>(parsed_prefix_length);
            } else {
                struct in_addr addr4;
                if (inet_pton(AF_INET, network_str.c_str(), &addr4) == 1) {
                    uint8_t network_bytes[16] = {};
                    std::memcpy(network_bytes + 12, &addr4, 4);
                    std::memcpy(&network_address, network_bytes, 16);

                    // IPv4 case
                    is_ipv6 = false;
                    size_t parsed_chars = 0;
                    int parsed_prefix_length = std::stoi(prefix_length_str, &parsed_chars);
                    if(parsed_chars != prefix_length_str.length()) throw std::invalid_argument("Invalid IPv4 prefix length");
                    if(parsed_prefix_length < 0 || parsed_prefix_length > 32) throw std::invalid_argument("Invalid IPv4 prefix length");
                    prefix_length = static_cast<uint32_t>(parsed_prefix_length + 96);
                } else {
                    throw std::invalid_argument("Invalid CIDR address");
                }
            }
            return Subnet(reverseByteOrder(network_address), prefix_length, is_ipv6);
        } catch(...) {
            std::cerr << "ERROR: Unable to parse cidr subnet " << subnet_cidr << "." << std::endl;
            std::exit(1);
        }
    }

    std::string toString() const {
        std::string subnet_str;
        char addr_str[INET6_ADDRSTRLEN];
        if (is_ipv6_) {
            // IPv6 case
            struct in6_addr addr6;
            for (int i = 0; i < 16; ++i) {
                addr6.s6_addr[i] = (network_ >> (8 * (15 - i))) & 0xFF;
            }
            inet_ntop(AF_INET6, &addr6, addr_str, INET6_ADDRSTRLEN);
            subnet_str = addr_str;
            subnet_str += "/" + std::to_string(prefix_length_);
        } else {
            // IPv4 case
            struct in_addr addr4;
            addr4.s_addr = htonl(static_cast<uint32_t>(network_));
            inet_ntop(AF_INET, &addr4, addr_str, INET_ADDRSTRLEN);
            subnet_str = addr_str;
            subnet_str += "/" + std::to_string(prefix_length_-96);
        }

        return subnet_str;
    }

private:
    uint128_t network_;
    uint32_t prefix_length_;
    bool is_ipv6_;
};

void optimizeSubnets(std::vector<Subnet> &subnets) {
    std::sort(subnets.begin(), subnets.end(), Subnet::sortDescendingPrecedence);
    bool found_optimization = true;
    while(found_optimization) {
        found_optimization = false;
        for(size_t i = 0; i<subnets.size() && !found_optimization; ++i) {
            for(size_t j = 0; j<subnets.size() && !found_optimization; ++j) {
                if(i != j) {
                    if(subnets[i].equals(subnets[j])) {
                        subnets.erase(std::next(subnets.begin(), j));
                        found_optimization = true;
                    } else
                    if(subnets[i].contains(subnets[j])) {
                        subnets.erase(std::next(subnets.begin(), j));
                        found_optimization = true;
                    } else
                    if(subnets[j].contains(subnets[i])) {
                        subnets.erase(std::next(subnets.begin(), i));
                        found_optimization = true;
                    } else
                    if(Subnet::canMerge(subnets[i], subnets[j])) {
                        Subnet merged_subnet = Subnet::merge(subnets[i], subnets[j]);
                        if(i>j) {
                            subnets.erase(std::next(subnets.begin(), i));
                            subnets.erase(std::next(subnets.begin(), j));
                        } else {
                            subnets.erase(std::next(subnets.begin(), j));
                            subnets.erase(std::next(subnets.begin(), i));
                        }
                        subnets.push_back(merged_subnet);
                        found_optimization = true;
                    }
                }
            }
        }
    }
    std::sort(subnets.begin(), subnets.end(), Subnet::sortDescendingPrecedence);
}

void subtractSubnets(std::vector<Subnet> &included_subnets, std::vector<Subnet> &excluded_subnets) {
    optimizeSubnets(excluded_subnets);

    bool found_collision = true;
    while(found_collision) {
        found_collision = false;
        for(size_t i = 0; i<included_subnets.size() && !found_collision; ++i) {
            for(size_t j = 0; j<excluded_subnets.size() && !found_collision; ++j) {
                if(included_subnets[i].equals(excluded_subnets[j])) {
                    included_subnets.erase(std::next(included_subnets.begin(), i));
                    found_collision = true;
                } else
                if(excluded_subnets[j].contains(included_subnets[i])) {
                    included_subnets.erase(std::next(included_subnets.begin(), i));
                    found_collision = true;
                } else
                if(included_subnets[i].contains(excluded_subnets[j])) {
                    auto [subnet1, subnet2] = included_subnets[i].split();
                    included_subnets.erase(std::next(included_subnets.begin(), i));
                    included_subnets.push_back(subnet1);
                    included_subnets.push_back(subnet2);
                    found_collision = true;
                }
            }
        }
    }

    optimizeSubnets(included_subnets);
}

int main(int argc, char* argv[]) {
    // Check if command-line arguments are provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [{+|-}]subnet1 [[{+|-}]subnet2] ..." << std::endl;
        std::exit(1);
    }

    // Vector to store current resulting subnets
    std::vector<Subnet> included_subnets;

    // Vector to store pending consecutive excluded subnets
    std::vector<Subnet> excluded_subnets;

    auto flushExcludedSubnets = [&]() {
        if(excluded_subnets.empty()) return;
        subtractSubnets(included_subnets, excluded_subnets);
        excluded_subnets.clear();
    };

    // Parse and process each included or excluded subnet provided as a command-line argument
    int IPvXmode = 0;
    bool has_explicit_include = false;

    for (int i = 1; i < argc; ++i) {
        std::string subnet_cidr = argv[i];
        bool include_subnet = true;
        if(subnet_cidr.length()>0) {
            if(subnet_cidr[0] == '+') {
                include_subnet = true;
                subnet_cidr = subnet_cidr.substr(1);
            } else
            if(subnet_cidr[0] == '-') {
                include_subnet = false;
                subnet_cidr = subnet_cidr.substr(1);
            }
        }
        Subnet new_subnet = Subnet::parseCidr(subnet_cidr);
        switch (IPvXmode)
        {
        case 4:
            if(new_subnet.isIPv6()) {
                std::cerr << "ERROR: Unable to add IPv6 subnet after IPv4 subnet: " << subnet_cidr << "." << std::endl;
                std::exit(1);
            }
            break;

        case 6:
            if(!new_subnet.isIPv6()) {
                std::cerr << "ERROR: Unable to add IPv4 subnet after IPv6 subnet: " << subnet_cidr << "." << std::endl;
                std::exit(1);
            }
            break;

        default:
            IPvXmode = new_subnet.isIPv6()?6:4;
            break;
        }
        if(include_subnet) {
            flushExcludedSubnets();
            included_subnets.push_back(new_subnet);
            optimizeSubnets(included_subnets);
            has_explicit_include = true;
        } else {
            excluded_subnets.push_back(new_subnet);
        }
    }

    // Add some good defaults for included subnets if no explicit includes were provided
    if(!has_explicit_include) {
        switch (IPvXmode) {
            case 4:
                included_subnets.push_back(Subnet::parseCidr("10.0.0.0/8"));
                included_subnets.push_back(Subnet::parseCidr("172.16.0.0/12"));
                included_subnets.push_back(Subnet::parseCidr("192.168.0.0/16"));
                break;
            case 6:
                included_subnets.push_back(Subnet::parseCidr("fc00::/7"));
                break;
        }
    }

    flushExcludedSubnets();

    // Ensure final result is normalized before presentation
    optimizeSubnets(included_subnets);

    // Sort calculated subnets nicely for printing
    std::sort(included_subnets.begin(), included_subnets.end(), Subnet::sortPrintNicely);

    // Print the calculated subnets
    std::cout << "Resulting Subnets:" << std::endl;
    for (const auto& subnet : included_subnets) {
        std::cout << "= " << subnet.toString() << std::endl;
    }

    return 0;
}

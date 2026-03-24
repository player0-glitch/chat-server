#ifndef PROTO_HPP
#define PROTO_HPP

#include <array>
#include <cstdint>
#include <string_view>

#define ARGS_IMPL
#include "args.hpp"

using std::array;
using std::string_view;

// Message types
#define MSG_BROADCAST 1
#define MSG_DM 2
#define MAX_FDS 35000
#define MAX_BUFF 1058
#define MAX_USRNAME_LEN 32
#define MAX_MSG_LEN 1024

// Effective struct padding for CPU and network stack
// stacking struct members from largest to smalled prevents having holes in the struct

#pragma pack(push, 1)
struct message_t {
    std::array<char, MAX_USRNAME_LEN> sender{};
    std::array<char, MAX_USRNAME_LEN> target{};
    uint32_t payload_len; // htonl or nthol will be needed
    uint8_t type;         // 1 for broadcast, 2 for DM
};
#pragma pack(pop)

// Although these won't go through the tcp pipe they are still efficiently packed
struct client_t {
    char username[MAX_USRNAME_LEN];
    int fd{};
    int is_active = -1;
};

void serialise_msg(std::array<char, MAX_USRNAME_LEN> &msg_fields, string_view val);
bool found_word_with_target(string_view target_symbol, string_view msg, size_t &pos);
#endif //! PROTO_HPP

// To keep the principle of a header only library without happing naming conflict
// is name manglig i decided to use header guards

#ifdef PROTO_IMPL

#include <algorithm>
void serialise_msg(std::array<char, MAX_USRNAME_LEN> &msg_fields, string_view val) {
    // copy string d ata needed for message_t frame
    size_t field_length = std::min(val.size(), msg_fields.size() - 1);
    e std::copy_n(val.begin(), field_length, msg_fields.begin());

    // manaully terminate with nulls
    msg_fields[field_length] = '\0';
};

/**
 * @brief the word with the given target_symbol
 * this method used to find the word with either '@' or '!q' which have defined
 * behaviour in this program
 * @param target_symbol is the symbol that will be targeted by the method
 * @param msg is the string that will be searched
 * @param pos this is only used for 'send_DM' method to make the search string
 * shorter
 */
bool found_word_with_target(string_view target_symbol, string_view msg, size_t &pos) {

    // We will ignore the pos parameter when this function is called by the
    // client_disconnect function
    // However, for the send_dm feature, it'll give a starting point to
    // find the user being DM'd

    //  find the position of the target target_symbol
    auto at = msg.find(target_symbol);

    // npos is a property so it this context it should return index of our
    // found target
    if (at != string_view::npos) {
        pos = at; // this will mark the beginning of the search
#if HAS_PRINTLN
        std::println("Found Target as {}", target_symbol);
#else
        std::cout << "Found Target as " << target_symbol << "\n";
#endif
        return true;
    }
    return false;
}
#endif

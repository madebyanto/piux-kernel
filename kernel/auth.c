#include <stdint.h>
#include "auth.h"
#include "ext2.h"
#include "keyboard.h"

#define AUTH_RECORD_SIZE 256
#define AUTH_PASSWORD_SIZE 64
#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

static char current_username[AUTH_USERNAME_SIZE];
static int current_sudoer;

static uint32_t rotate_right(uint32_t value, uint32_t amount) {
    return (value >> amount) | (value << (32 - amount));
}

static uint32_t read_word(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | data[3];
}

static void write_word(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static void sha256_transform(uint32_t state[8], const uint8_t block[SHA256_BLOCK_SIZE]) {
    static const uint32_t constants[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    uint32_t words[64];
    uint32_t a, b, c, d, e, f, g, h;
    for (int i = 0; i < 16; i++) words[i] = read_word(block + i * 4);
    for (int i = 16; i < 64; i++) {
        uint32_t small_sigma0 = rotate_right(words[i - 15], 7) ^ rotate_right(words[i - 15], 18) ^ (words[i - 15] >> 3);
        uint32_t small_sigma1 = rotate_right(words[i - 2], 17) ^ rotate_right(words[i - 2], 19) ^ (words[i - 2] >> 10);
        words[i] = words[i - 16] + small_sigma0 + words[i - 7] + small_sigma1;
    }
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];
    for (int i = 0; i < 64; i++) {
        uint32_t big_sigma0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
        uint32_t big_sigma1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
        uint32_t choose = (e & f) ^ ((~e) & g);
        uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temporary1 = h + big_sigma1 + choose + constants[i] + words[i];
        uint32_t temporary2 = big_sigma0 + majority;
        h = g; g = f; f = e; e = d + temporary1;
        d = c; c = b; b = a; a = temporary1 + temporary2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256(const char *input, char output[65]) {
    uint8_t block[SHA256_BLOCK_SIZE];
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t length = 0;
    uint32_t offset = 0;
    static const char hex[] = "0123456789abcdef";
    while (input[length]) length++;
    while (length - offset >= SHA256_BLOCK_SIZE) {
        sha256_transform(state, (const uint8_t *)input + offset);
        offset += SHA256_BLOCK_SIZE;
    }
    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) block[i] = 0;
    for (uint32_t i = offset; i < length; i++) block[i - offset] = (uint8_t)input[i];
    block[length - offset] = 0x80;
    if (length - offset >= 56) {
        sha256_transform(state, block);
        for (int i = 0; i < SHA256_BLOCK_SIZE; i++) block[i] = 0;
    }
    uint64_t bit_length = (uint64_t)length * 8;
    for (int i = 0; i < 8; i++) block[63 - i] = (uint8_t)(bit_length >> (i * 8));
    sha256_transform(state, block);
    for (int i = 0; i < 8; i++) {
        uint8_t digest[4];
        write_word(digest, state[i]);
        for (int j = 0; j < 4; j++) {
            output[(i * 4 + j) * 2] = hex[digest[j] >> 4];
            output[(i * 4 + j) * 2 + 1] = hex[digest[j] & 0x0f];
        }
    }
    output[64] = '\0';
}

static int string_length(const char *text) {
    int length = 0;
    while (text[length]) length++;
    return length;
}

static int strings_equal(const char *left, const char *right) {
    int index = 0;
    while (left[index] && right[index] && left[index] == right[index]) index++;
    return left[index] == right[index];
}

static int write_auth_record(const char *record) {
    int inode = ext2_find_inode_by_path("/.config/passwd");
    char existing[AUTH_RECORD_SIZE];
    int length = 0;
    int record_length = string_length(record);
    if (inode >= 0) {
        length = ext2_read_file((uint32_t)inode, existing, sizeof(existing) - 1);
        if (length < 0) return -1;
    } else {
        if (ext2_create_file_by_path("/.config/passwd") < 0) return -1;
        inode = ext2_find_inode_by_path("/.config/passwd");
    }
    if (length + record_length >= (int)sizeof(existing)) return -1;
    for (int i = 0; i < record_length; i++) existing[length + i] = record[i];
    length += record_length;
    return ext2_write_file((uint32_t)inode, existing, (uint32_t)length) < 0 ? -1 : 0;
}

int auth_create_user(const char *username, const char *password, int sudoer) {
    char password_hash[65];
    char record[AUTH_RECORD_SIZE];
    int offset = 0;
    sha256(password, password_hash);
    for (int i = 0; username[i] && offset < AUTH_RECORD_SIZE - 1; i++) record[offset++] = username[i];
    record[offset++] = ':';
    for (int i = 0; password_hash[i] && offset < AUTH_RECORD_SIZE - 1; i++) record[offset++] = password_hash[i];
    record[offset++] = ':';
    record[offset++] = sudoer ? '1' : '0';
    record[offset++] = '\n';
    record[offset] = '\0';
    return write_auth_record(record);
}

static int read_input(auth_puts_t puts, auth_putc_t putc, const char *label,
                      char *buffer, int size, int secret) {
    int length = 0;
    puts(label);
    while (1) {
        int key = keyboard_read_char();
        if (key == '\n') {
            buffer[length] = '\0';
            putc('\n');
            return length;
        }
        if (key == '\b' && length > 0) {
            length--;
            putc('\b');
        } else if (key >= 32 && key < 127 && length < size - 1) {
            buffer[length++] = (char)key;
            putc(secret ? '*' : (char)key);
        }
    }
}

static int authenticate(const char *username, const char *password, int *sudoer) {
    char records[512];
    char password_hash[65];
    int bytes = ext2_read_file_by_path("/.config/passwd", records, sizeof(records) - 1);
    int offset = 0;
    if (bytes < 0) return -1;
    records[bytes] = '\0';
    sha256(password, password_hash);
    while (offset < bytes) {
        char stored_username[AUTH_USERNAME_SIZE];
        char stored_hash[65];
        int user_length = 0;
        int hash_length = 0;
        while (offset < bytes && records[offset] != ':' && user_length < AUTH_USERNAME_SIZE - 1) {
            stored_username[user_length++] = records[offset++];
        }
        stored_username[user_length] = '\0';
        if (offset >= bytes || records[offset++] != ':') break;
        while (offset < bytes && records[offset] != ':' && hash_length < 64) stored_hash[hash_length++] = records[offset++];
        stored_hash[hash_length] = '\0';
        if (offset >= bytes || records[offset++] != ':') break;
        *sudoer = offset < bytes && records[offset++] == '1';
        while (offset < bytes && records[offset] != '\n') offset++;
        if (offset < bytes) offset++;
        if (strings_equal(username, stored_username) && strings_equal(password_hash, stored_hash)) return 0;
    }
    return -1;
}

int auth_login(auth_clear_t clear, auth_puts_t puts, auth_putc_t putc) {
    char username[AUTH_USERNAME_SIZE];
    char password[AUTH_PASSWORD_SIZE];
    int sudoer;
    while (1) {
        clear();
        puts("Piux login\n\n");
        if (read_input(puts, putc, "Username: ", username, sizeof(username), 0) == 0) continue;
        read_input(puts, putc, "Password: ", password, sizeof(password), 1);
        if (authenticate(username, password, &sudoer) == 0) {
            int index = 0;
            while (username[index] && index < AUTH_USERNAME_SIZE - 1) {
                current_username[index] = username[index];
                index++;
            }
            current_username[index] = '\0';
            current_sudoer = sudoer;
            clear();
            return 0;
        }
        puts("\nLogin failed. Press Enter to try again.");
        keyboard_read_char();
    }
}

const char *auth_current_username(void) { return current_username; }
int auth_current_user_is_sudoer(void) { return current_sudoer; }

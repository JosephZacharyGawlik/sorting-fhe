#include <seal/seal.h>
#include <iostream>
#include <vector>
#include <chrono>

using namespace std;
using namespace seal;

/*******************************************
 * GLOBAL COUNTERS
 *******************************************/
size_t rotation_count = 0;
size_t multiplication_count = 0;

/*******************************************
 * ENUM
 *******************************************/
enum class SortType {
    Bitonic,
    Batcher,
    RankBased
};

/*******************************************
 * ROTATION WRAPPER
 *******************************************/
Ciphertext rotate_ct(const Ciphertext &ct,
                     int shift,
                     Evaluator &evaluator,
                     const GaloisKeys &galois_keys)
{
    Ciphertext result;
    evaluator.rotate_rows(ct, shift, galois_keys, result);
    rotation_count++;
    return result;
}

/*******************************************
 * PLACEHOLDER COMPARE-AND-SWAP
 *
 * This is NOT final comparator.
 * It just performs arithmetic to test
 * rotations + multiplications + depth.
 *******************************************/
void compare_and_swap(Ciphertext &ct,
                      int shift,
                      Evaluator &evaluator,
                      const GaloisKeys &galois_keys,
                      const RelinKeys &relin_keys)
{
    Ciphertext rotated = rotate_ct(ct, shift, evaluator, galois_keys);

    Ciphertext diff;
    evaluator.sub(ct, rotated, diff);

    Ciphertext diff_sq;
    evaluator.square(diff, diff_sq);
    evaluator.relinearize_inplace(diff_sq, relin_keys);
    multiplication_count++;

    Ciphertext temp;
    evaluator.add(ct, diff_sq, temp);

    ct = temp;
}

/*******************************************
 * BITONIC SORT
 *******************************************/
void bitonic_sort(Ciphertext &ct,
                  size_t n,
                  Evaluator &evaluator,
                  const GaloisKeys &galois_keys,
                  const RelinKeys &relin_keys)
{
    for (size_t k = 2; k <= n; k <<= 1) {
        for (size_t j = k >> 1; j > 0; j >>= 1) {
            compare_and_swap(ct, j, evaluator, galois_keys, relin_keys);
        }
    }
}

/*******************************************
 * BATCHER ODD–EVEN MERGE SORT
 *******************************************/
void batcher_sort(Ciphertext &ct,
                  size_t n,
                  Evaluator &evaluator,
                  const GaloisKeys &galois_keys,
                  const RelinKeys &relin_keys)
{
    for (size_t p = 1; p < n; p <<= 1) {
        for (size_t k = p; k >= 1; k >>= 1) {
            compare_and_swap(ct, k, evaluator, galois_keys, relin_keys);
            if (k == 1) break;
        }
    }
}

/*******************************************
 * RANK-BASED SORT
 *
 * Each element compared with all others
 *******************************************/
void rank_based_sort(Ciphertext &ct,
                     size_t n,
                     Evaluator &evaluator,
                     const GaloisKeys &galois_keys,
                     const RelinKeys &relin_keys)
{
    for (size_t i = 1; i < n; i++) {
        compare_and_swap(ct, i, evaluator, galois_keys, relin_keys);
    }
}

/*******************************************
 * MAIN
 *******************************************/
int main()
{
    /*********************
     * FAST BFV SETUP
     *********************/
    size_t poly_modulus_degree = 4096;

    EncryptionParameters parms(scheme_type::bfv);
    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::BFVDefault(poly_modulus_degree));
    parms.set_plain_modulus(PlainModulus::Batching(poly_modulus_degree, 16));

    SEALContext context(parms);

    KeyGenerator keygen(context);
    auto secret_key = keygen.secret_key();

    PublicKey public_key;
    keygen.create_public_key(public_key);

    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);

    GaloisKeys galois_keys;
    keygen.create_galois_keys(galois_keys);

    Encryptor encryptor(context, public_key);
    Evaluator evaluator(context);
    Decryptor decryptor(context, secret_key);
    BatchEncoder encoder(context);

    /*********************
     * SMALL SANITY INPUT
     *********************/
    size_t n = 4;
    size_t slot_count = encoder.slot_count();

    vector<uint64_t> input(slot_count, 0ULL);

    input[0] = 7;
    input[1] = 2;
    input[2] = 5;
    input[3] = 1;

    cout << "Plain input: ";
    for (size_t i = 0; i < n; i++)
        cout << input[i] << " ";
    cout << endl;

    Plaintext pt;
    encoder.encode(input, pt);

    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    /*********************
     * SELECT SORT TYPE
     *********************/
    SortType sort_type = SortType::Bitonic;
    // Change here:
    // SortType::Batcher
    // SortType::RankBased

    auto start = chrono::high_resolution_clock::now();

    switch (sort_type) {
        case SortType::Bitonic:
            bitonic_sort(ct, n, evaluator, galois_keys, relin_keys);
            break;
        case SortType::Batcher:
            batcher_sort(ct, n, evaluator, galois_keys, relin_keys);
            break;
        case SortType::RankBased:
            rank_based_sort(ct, n, evaluator, galois_keys, relin_keys);
            break;
    }

    auto end = chrono::high_resolution_clock::now();

    /*********************
     * BENCHMARK INFO
     *********************/
    auto runtime =
        chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "Runtime (ms): " << runtime << endl;
    cout << "Rotations: " << rotation_count << endl;
    cout << "Multiplications: " << multiplication_count << endl;
    cout << "Noise budget: "
         << decryptor.invariant_noise_budget(ct)
         << " bits" << endl;

    /*********************
     * DECRYPT
     *********************/
    Plaintext result;
    decryptor.decrypt(ct, result);

    vector<uint64_t> output;
    encoder.decode(result, output);

    cout << "Decrypted output: ";
    for (size_t i = 0; i < n; i++)
        cout << output[i] << " ";
    cout << endl;

    return 0;
}
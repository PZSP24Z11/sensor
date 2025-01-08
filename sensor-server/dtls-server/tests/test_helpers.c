#include <stdlib.h>
#include <check.h>
#include "../helpers.h"

/*---------------------------|VALIDATE SENSOR REQUEST TESTS|-----------------------------*/

START_TEST(ok_normal_case) {
    const char sreq[] = "SENSORREQ%00:11:22:33:44:55%TH";
    ck_assert(validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(ok_hexadecimal_mac) {
    const char sreq[] = "SENSORREQ\%ff:a1:2b:3c:d4:e5%TH";
    ck_assert(validate_sreq(sreq, strlen(sreq)));
}

START_TEST(ok_one_measurement_type) {
    const char sreq[] = "SENSORREQ%05:a1:2b:3c:d4:e5%T";
    ck_assert(validate_sreq(sreq, strlen(sreq)));
}

START_TEST(ok_many_measurement_types) {
    const char sreq[] = "SENSORREQ%05:a1:2b:3c:d4:e5%THPW";
    ck_assert(validate_sreq(sreq, strlen(sreq)));
}

START_TEST(invalid_incomplete_request_name) {
    const char sreq[] = "SENSORE%00:11:22:33:44:55%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_bad_request_name) {
    const char sreq[] = "completely_invalid name%00:11:22:33:44:55%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_bad_separator) {
    const char sreq[] = "SENSORREQ;00:11:22:33:44:55;TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_characters) {
    const char sreq[] = "SENSORREQ%00:11:22:33:44:ZZ%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_no_separators) {
    const char sreq[] = "SENSORREQ%\a0h12d334d55%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_too_long) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4:e5:90%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_too_short) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_badly_separated) {
    const char sreq[] = "SENSORREQ%\ffa:1:2b3:cd:d40%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_mac_wrong_separator) {
    const char sreq[] = "SENSORREQ%\ff;a1;r2;b3;cd;d4%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_measurement_types_lowercase) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4:99%thp";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_measurement_types_special_chars) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4:73%t!hp";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_measurement_types_empty) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4:73%";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_request_no_measurement_types_part) {
    const char sreq[] = "SENSORREQ%\ff:a1:2b:3c:d4:73";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_request_no_mac) {
    const char sreq[] = "SENSORREQ%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

START_TEST(invalid_request_no_sensorreq) {
    const char sreq[] = "ff:a1:2b:3c:d4:e5%TH";
    ck_assert(!validate_sreq(sreq, strlen(sreq)));
}
END_TEST

/*-----------------------------|VALIDATE MEASUREMENTS TESTS|-----------------------------*/

START_TEST(ok_single_measurement) {
    const char measurements[] = "T1770";
    ck_assert(validate_measurements(measurements, strlen(measurements)));
}

START_TEST(ok_two_measurements) {
    const char measurements[] = "T1770%H0080";
    ck_assert(validate_measurements(measurements, strlen(measurements)));
}

START_TEST(ok_many_measurements) {
    const char measurements[] = "T1770%H0080%P3000%I3321";
    ck_assert(validate_measurements(measurements, strlen(measurements)));
}
START_TEST(invalid_measurement_length) {
    const char measurements[] = "T177";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

START_TEST(invalid_measurement_prefix_lowercase) {
    const char measurements[] = "t1770";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

START_TEST(invalid_measurement_prefix_special_character) {
    const char measurements[] = "T1770%!1234";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

START_TEST(invalid_measurement_digits) {
    const char measurements[] = "T17a0";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

START_TEST(invalid_measurement_separator) {
    const char measurements[] = "T1770;H0080";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

START_TEST(invalid_measurement_prefix_percent_character) {
    const char measurements[] = "T1770%\%1234";
    ck_assert(!validate_measurements(measurements, strlen(measurements)));
}
END_TEST

Suite* validate_sreq_suite(void) {
    Suite *s;
    TCase *tc_core;

    tc_core = tcase_create("Core");
    s = suite_create("Validate Sensor Request - validate_sreq()");

    tcase_add_test(tc_core, ok_normal_case);
    tcase_add_test(tc_core, ok_hexadecimal_mac);
    tcase_add_test(tc_core, ok_one_measurement_type);
    tcase_add_test(tc_core, invalid_incomplete_request_name);
    tcase_add_test(tc_core, invalid_bad_request_name);
    tcase_add_test(tc_core, invalid_bad_separator);
    tcase_add_test(tc_core, invalid_mac_characters);
    tcase_add_test(tc_core, invalid_mac_no_separators);
    tcase_add_test(tc_core, invalid_mac_too_long);
    tcase_add_test(tc_core, invalid_mac_too_short);
    tcase_add_test(tc_core, invalid_mac_wrong_separator);
    tcase_add_test(tc_core, invalid_mac_badly_separated);
    tcase_add_test(tc_core, invalid_measurement_types_lowercase);
    tcase_add_test(tc_core, invalid_measurement_types_special_chars);
    tcase_add_test(tc_core, invalid_measurement_types_empty);
    tcase_add_test(tc_core, invalid_request_no_measurement_types_part);

    suite_add_tcase(s, tc_core);

    return s;
}

Suite *validate_measurements_suite(void) {
    Suite *s;
    TCase *tc_core;

    tc_core = tcase_create("Core");
    s = suite_create("Validate Sensor Measurements - validate_measurements()");

    tcase_add_test(tc_core, ok_single_measurement);
    tcase_add_test(tc_core, ok_two_measurements);
    tcase_add_test(tc_core, ok_many_measurements);
    tcase_add_test(tc_core, invalid_measurement_length);
    tcase_add_test(tc_core, invalid_measurement_prefix_lowercase);
    tcase_add_test(tc_core, invalid_measurement_prefix_special_character);
    tcase_add_test(tc_core, invalid_measurement_digits);
    tcase_add_test(tc_core, invalid_measurement_separator);
    tcase_add_test(tc_core, invalid_measurement_prefix_percent_character);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = validate_sreq_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    s = validate_measurements_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);

    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);


    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

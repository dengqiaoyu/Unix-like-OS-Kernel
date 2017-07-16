/**
 * @file util_asn.S
 * @brief Function implementation of getting current ebp.
 * @author Qiaoyu Deng
 * @andrew_id qdeng
 * @bugs No known bugs
 */

.global get_ebp

get_ebp:
    movl %ebp, %eax
    ret

#pragma once
#include "compression_db2.h"

prefixOptimization::prefixOptimization() {
    memusage = 0;
    base = NewPage();
    newsize = new uint8_t[kNumberBound];
    newoffset = new uint16_t[kNumberBound];
}

prefixOptimization::~prefixOptimization() {
    delete base;
    delete newsize;
    delete newoffset;
}

closedRange find_closed_range(vector<PrefixMetaData> prefixmetadatas,
                              const char *newprefix, int prefixlen, int p_i_pos) {
    closedRange closedRange;
    if (prefixlen == 0) {
        return closedRange;
    }
    int i;
    for (i = p_i_pos - 1; i >= 0; i--) {
        const char *p_prev = prefixmetadatas[i].prefix->addr();
        if (strncmp(p_prev, newprefix, prefixlen) != 0)
            break;
    }
    i++;
    closedRange.pos_low = i;
    for (; i < p_i_pos; i++) {
        closedRange.prefixMetadatas.push_back(prefixmetadatas[i]);
        // positions.push_back(i);
    }
    // // Reverse for ease of use
    // reverse(closedRangePrefixes.begin(), closedRangePrefixes.end());
    // reverse(positions.begin(), positions.end());

    // closedRange.prefixMetadatas = closedRangePrefixes;
    closedRange.pos_high = p_i_pos - 1;
    return closedRange;
}

// int search_prefix_metadata(DB2Node *cursor, string_view key) {
//     // binary search
//     vector<PrefixMetaData> prefixMetadatas = cursor->prefixMetadata;
//     int low = 0;
//     int high = prefixMetadatas.size() - 1;
//     int prefixcmp = -1;
//     while (low <= high) {
//         int mid = low + (high - low) / 2;
//         PrefixMetaData midMetadata = prefixMetadatas.at(mid);
//         string prefix = midMetadata.prefix;
//         int prefixlen = prefix.length();
//         prefixcmp = key.compare(0, prefixlen, prefix);
//         int cmp = prefixcmp;
//         if (prefixcmp == 0) {
//             int prefixlow = midMetadata.low;
//             int prefixhigh = midMetadata.high;
//             int len = key.length() - prefixlen;
//             int lowcmp = key.compare(prefixlen, len, cursor->keys.at(prefixlow).value);
//             int highcmp = key.compare(prefixlen, len, cursor->keys.at(prefixhigh).value);
//             cmp = (lowcmp >= 0) ^ (highcmp <= 0) ? lowcmp : 0;
//         }
//         if (cmp == 0)
//             return mid;
//         else if (cmp > 0)
//             low = mid + 1;
//         else
//             high = mid - 1;
//     }
//
//     return -1;
// }

int find_prefix_pos(DB2Node *cursor, const char *key, int keylen, bool for_insert_or_nonleaf) {
    vector<PrefixMetaData> prefixes = cursor->prefixMetadata;
    int low = 0;
    int high = prefixes.size() - 1;
    while (low <= high) {
        int cur = low + (high - low) / 2;
        PrefixMetaData curMetadata = prefixes[cur];
        Data *prefix = curMetadata.prefix;
        // int prefixlen = prefix.length();
        // int prefixcmp = key.compare(0, prefixlen, prefix);
        int prefixcmp = strncmp(key, prefix->addr(), prefix->size);
        int cmp = prefixcmp;
        if (prefixcmp == 0) {
            // int prefixlow = curMetadata.low;
            // int prefixhigh = midMetadata.high;
            // int len = keylen - prefix->size;
            keylen -= prefix->size;
            const char *key_cutoff = key + prefix->size;
            int lowcmp = strncmp(key_cutoff, GetKey(cursor, curMetadata.low), keylen);
            int highcmp = strncmp(key_cutoff, GetKey(cursor, curMetadata.high), keylen);
            // int lowcmp = key.compare(prefixlen, len, cursor->keys.at(prefixlow).value);
            // int highcmp = key.compare(prefixlen, len, cursor->keys.at(prefixhigh).value);
            cmp = (lowcmp >= 0) ^ (highcmp <= 0) ? lowcmp : 0;
        }
        if (cmp == 0)
            return cur;
        else if (cmp > 0)
            low = cur + 1;
        else
            high = cur - 1;
    }
    if (!for_insert_or_nonleaf)
        return -1;
    // Not found ?
    int prevcmp = -1;
    if (high >= 0 && high < prefixes.size()) {
        Data *prevprefix = prefixes[high].prefix;
        prevcmp = strncmp(key, prevprefix->addr(), prevprefix->size);
        // prevcmp = key.compare(0, prevprefix.length(), prevprefix);
    }
    return prevcmp == 0 ? high : high + 1;
}

// db2split perform_split(DB2Node *node, int split, bool isleaf) {
//     vector<PrefixMetaData> metadatas = node->prefixMetadata;
//     vector<PrefixMetaData> leftmetadatas;
//     vector<PrefixMetaData> rightmetadatas;
//     PrefixMetaData leftmetadata, rightmetadata;
//     int rightindex = 0;
//     string splitprefix;
//     db2split result;
//     for (uint32_t i = 0; i < metadatas.size(); i++) {
//         if (split >= metadatas.at(i).low && split <= metadatas.at(i).high) {
//             splitprefix = metadatas.at(i).prefix;
//
//             if (split != metadatas.at(i).low) {
//                 leftmetadata = PrefixMetaData(metadatas.at(i).prefix, metadatas.at(i).low, split - 1);
//                 leftmetadatas.push_back(leftmetadata);
//             }
//
//             int rsize = isleaf ? metadatas.at(i).high - split : metadatas.at(i).high - (split + 1);
//             if (rsize >= 0) {
//                 rightmetadata = PrefixMetaData(metadatas.at(i).prefix, rightindex, rightindex + rsize);
//                 rightindex = rightindex + rsize + 1;
//                 rightmetadatas.push_back(rightmetadata);
//             }
//         }
//         else if (split <= metadatas.at(i).low) {
//             int size = metadatas.at(i).high - metadatas.at(i).low;
//             rightmetadata = PrefixMetaData(metadatas.at(i).prefix,
//                                            rightindex, rightindex + size);
//             rightindex = rightindex + size + 1;
//             rightmetadatas.push_back(rightmetadata);
//         }
//         else {
//             leftmetadata = metadatas.at(i);
//             leftmetadatas.push_back(leftmetadata);
//         }
//     }
//     result.leftmetadatas = leftmetadatas;
//     result.rightmetadatas = rightmetadatas;
//     result.splitprefix = splitprefix;
//     return result;
// }

// Cost of optimization is calculated as size of prefixes + size of suffixes
// int calculate_cost_of_optimization(prefixOptimization result) {
//     int cost = 0;
//     for (int i = 0; i < result.prefixMetadatas.size(); i++) {
//         int low = result.prefixMetadatas.at(i).low;
//         int high = result.prefixMetadatas.at(i).high;
//         cost += result.prefixMetadatas.at(i).prefix.length();
//         for (int j = low; j <= high; j++) {
//             cost += result.keys.at(j).getSize();
//         }
//     }
//     return cost;
// }

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized
int calculate_prefix_merge_cost(prefixOptimization *result, vector<PrefixMetaData> segment, Data *prefix) {
    int cost = prefix->size * segment.size();
    for (auto s : segment) {
        for (int i = s.low; i <= s.high; i++) {
            cost += s.prefix->size + result->newsize[i] - prefix->size;
        }
    }
    // for (uint32_t i = 0; i < segment.size(); i++) {
    //     int low = segment[i].low;
    //     int high = segment[i].high;
    //     for (int j = low; j <= high; j++) {
    //         string decompressed = segment.at(i).prefix + keys.at(j).value;
    //         string compressedkey = decompressed.substr(prefix.length());
    //         cost += compressedkey.length();
    //     }
    // }
    return cost;
}

prefixMergeSegment find_best_segment_of_size_k(prefixOptimization *result, closedRange closedRange, int k) {
    prefixMergeSegment result_seg;
    vector<PrefixMetaData> bestsegment;
    vector<PrefixMetaData> currsegment;
    // Data *bestprefix;
    Data *currprefix = new Data("", 0);
    // int bestcost = INT32_MAX;
    int currcost = 0;
    int firstindex = 0;
    // int bestindex = 0;
    for (int i = closedRange.pos_low; i <= closedRange.pos_high; i++) {
        int idx = i - closedRange.pos_low;
        if (idx % k == 0) {
            currcost = calculate_prefix_merge_cost(result, currsegment, currprefix);
            firstindex = i;
            if (i != 0 && currcost < result_seg.cost) {
                result_seg.prefix = currprefix;
                result_seg.segment = currsegment;
                result_seg.cost = currcost;
                result_seg.firstindex = firstindex;
            }
            currprefix = closedRange.prefixMetadatas[idx].prefix;
            currsegment.clear();
        }
        else {
            // Only need to update the length?
            currprefix->size = get_common_prefix_len(currprefix->addr(), closedRange.prefixMetadatas[idx].prefix->addr(),
                                                     currprefix->size, closedRange.prefixMetadatas[idx].prefix->size);
        }
        currsegment.push_back(closedRange.prefixMetadatas[idx]);
    }
    // for (int i = 0; i < closedRange.prefixMetadatas.size(); i++) {
    //     if (i % k == 0) {
    //         currcost = calculate_prefix_merge_cost(keys, currsegment, currprefix);
    //         firstindex = closedRange.positions.at(i);
    //         if (i != 0 && currcost < bestcost) {
    //             bestprefix = currprefix;
    //             bestsegment = currsegment;
    //             bestcost = currcost;
    //             bestindex = firstindex;
    //         }
    //         currprefix = closedRange.prefixMetadatas.at(i).prefix;
    //         currsegment.clear();
    //     }
    //     else {
    //         currprefix = get_common_prefix(currprefix, closedRange.prefixMetadatas.at(i).prefix);
    //     }
    //     currsegment.push_back(closedRange.prefixMetadatas.at(i));
    // }
    if (currcost < result_seg.cost) {
        result_seg.prefix = currprefix;
        result_seg.segment = currsegment;
        result_seg.cost = currcost;
        result_seg.firstindex = firstindex;
    }
    // result_seg.segment = bestsegment;
    // result_seg.prefix = bestprefix;
    // result_seg.cost = bestcost;
    // result_seg.firstindex = bestindex;
    return result_seg;
}

prefixMergeSegment find_best_segment_in_closed_range(prefixOptimization *result, closedRange closedRange) {
    prefixMergeSegment bestsegment;
    int bestcost = INT32_MAX;
    // Start searching for segments of size 2
    for (int i = 2; i <= closedRange.prefixMetadatas.size(); i++) {
        prefixMergeSegment segment = find_best_segment_of_size_k(result, closedRange, i);
        if (segment.cost < bestcost) {
            bestcost = segment.cost;
            bestsegment = segment;
        }
    }
    return bestsegment;
}

void merge_prefixes_in_segment(prefixOptimization *result,
                               prefixMergeSegment bestsegment) {
    // the result.prefix should be shorten in this section

    // cout << "Best segment prefix " << bestsegment.prefix << endl;
    // cout << "Best segment first index " << bestsegment.firstindex << endl;
    // vector<PrefixMetaData> segment = bestsegment.segment;
    vector<PrefixMetaData> prefixes = bestsegment.segment;
    Data *newprefix = bestsegment.prefix;
    int firstpos = bestsegment.firstindex;
    int firstlow = bestsegment.segment[0].low;
    int lasthigh = 0;
    for (auto s : bestsegment.segment) {
        Data *prevprefix = s.prefix;
        lasthigh = s.high;
        for (int i = s.low; i <= s.high; i++) {
            // Merge means all the prefixes may be shorten, have to decompress
            // Or we only keep the keysize, don't modify the base anymore
            int extra_len = s.prefix->size - newprefix->size;
            result->newsize[i] += extra_len;
            result->newoffset[i] = extra_len;
        }
    }
    // for (uint32_t i = 0; i < bestsegment.segment.size(); i++) {
    //     Data *prevprefix = bestsegment.segment[i].prefix;
    //     int low = segment.at(i).low;
    //     int high = segment.at(i).high;
    //     lasthigh = high;
    //     for (int j = low; j <= high; j++) {
    //         string decompressed = prevprefix + keys.at(j).value;
    //         // Representing null prefix
    //         if (newprefix.compare("") == 0) {
    //             keys.at(j).value = string_to_char(decompressed);
    //         }
    //         else {
    //             keys.at(j).value = string_to_char(decompressed.substr(newprefix.length()));
    //         }
    //     }
    // }

    result->prefixes.erase(next(result->prefixes.begin(), firstpos),
                           next(result->prefixes.begin(), firstpos + bestsegment.segment.size()));
    PrefixMetaData newmetadata = PrefixMetaData(newprefix->addr(), newprefix->size, firstlow, lasthigh);
    result->prefixes.insert(result->prefixes.begin() + firstpos, newmetadata);
}

prefixOptimization *prefix_merge(DB2Node *node) {
    prefixOptimization *result = new prefixOptimization();
    result->memusage = 0;
    memcpy(result->base, node->base, sizeof(char) * MAX_SIZE_IN_BYTES);
    memset(result->newoffset, 0, sizeof(uint16_t) * kNumberBound);
    memcpy(result->newsize, node->keys_size, sizeof(uint8_t) * kNumberBound);
    result->prefixes = node->prefixMetadata;

    // vector<Key_c> keysCopy(keys);
    // vector<PrefixMetaData> metadataCopy(prefixmetadatas);

    int nextindex = 1;
    int numremoved;
    while (nextindex < result->prefixes.size()) {
        // string p_i = metadataCopy.at(nextindex).prefix;
        // string p_i_minus_1 = metadataCopy.at(nextindex - 1).prefix;
        // string p_i_first_key = p_i + keysCopy.at(metadataCopy.at(nextindex).low).value;
        // string p_i_prev_last_key = p_i_minus_1 + keysCopy.at(metadataCopy.at(nextindex - 1).high).value;
        // string n_p_i = get_common_prefix(p_i_first_key, p_i_prev_last_key);

        PrefixMetaData p_i = result->prefixes[nextindex];
        PrefixMetaData p_prev = result->prefixes[nextindex - 1];

        // TODO: here I guess we don't need to decompress the string,
        // Because the common prefix should be shorter than any existing prefixes
        int ll_len = p_prev.prefix->size + result->newsize[p_prev.high];
        int rf_len = p_i.prefix->size + result->newsize[p_i.low];
        char *leftlast = new char[ll_len + 1];
        char *rightfirst = new char[rf_len + 1];

        strcpy(leftlast, p_prev.prefix->addr());
        strcpy(leftlast + p_prev.prefix->size, GetKeyDB2ByPtr(result, p_prev.high));
        strcpy(rightfirst, p_i.prefix->addr());
        strcpy(rightfirst + p_i.prefix->size, GetKeyDB2ByPtr(result, p_i.low));

        // int prefixlen = get_common_prefix_len(leftlast, rightfirst, ll_len, rf_len);
        int prefixlen = get_common_prefix_len(p_prev.prefix->addr(), p_i.prefix->addr(),
                                              p_prev.prefix->size, p_i.prefix->size);
        // TODO: looks like don't need to care the suffix parts
        // the range is closed like [low, high], within the scope of [0:nextindex)
        // closedRange closedRange = find_closed_range(result->prefixes,
        //                                     rightfirst, prefixlen, nextindex);
        closedRange closedRange = find_closed_range(result->prefixes,
                                                    p_i.prefix->addr(), prefixlen, nextindex);

        numremoved = 0;
        if (closedRange.pos_high - closedRange.pos_low > 0) {
            // sizeof closedrange > 1

            // if (n_p_i.length() == 0) {
            //     prefixMergeSegment bestSegment = find_best_segment_in_closed_range(keysCopy, closedRange);
            //     merge_prefixes_in_segment(keysCopy, metadataCopy, bestSegment, "");
            //     numremoved = bestSegment.segment.size();
            // }
            if (prefixlen < p_prev.prefix->size) {
                prefixMergeSegment bestSegment = find_best_segment_in_closed_range(result, closedRange);
                merge_prefixes_in_segment(result, bestSegment);
                numremoved = bestSegment.segment.size();
            }
        }
        delete leftlast;
        delete rightfirst;
        nextindex = nextindex + 1 - numremoved;
    }
    // result.keys = keysCopy;
    // result.prefixMetadatas = metadataCopy;
    // int prefix_count = 0;
    for (auto pfx : result->prefixes) {
        result->memusage += pfx.prefix->size;
        // prefix_count += pfx.prefix->size;
        for (int i = pfx.low; i <= pfx.high; i++)
            result->memusage += result->newsize[i];
    }
    // cout << "merge: pfx: " << prefix_count << ", total:" << result->memusage << endl;
    return result;
}

int expand_prefixes_in_boundary(prefixOptimization *result, int index) {
    // update prefixmetadata and keys in the result
    PrefixMetaData p_i = result->prefixes[index];
    PrefixMetaData p_i_1 = result->prefixes[index + 1];
    // string common_prefix = get_common_prefix(p_i.prefix + keys.at(p_i.high).value,
    //                                          p_i_plus_1.prefix + keys.at(p_i_plus_1.low).value);

    int ll_len = p_i.prefix->size + result->newsize[p_i.high];
    int rf_len = p_i_1.prefix->size + result->newsize[p_i_1.low];
    char *leftlast = new char[ll_len + 1];
    char *rightfirst = new char[rf_len + 1];
    strcpy(leftlast, p_i.prefix->addr());
    strcpy(leftlast + p_i.prefix->size, GetKeyDB2ByPtr(result, p_i.high));
    strcpy(rightfirst, p_i_1.prefix->addr());
    strcpy(rightfirst + p_i_1.prefix->size, GetKeyDB2ByPtr(result, p_i_1.low));

    int prefixlen = get_common_prefix_len(leftlast, rightfirst, ll_len, rf_len);

    // int prefixlen = common_prefix.length();
    int firstlow = -1;
    int lasthigh = -1;

    // int memusage = 0;

    if (prefixlen > 0 && prefixlen >= max(p_i.prefix->size, p_i_1.prefix->size)) {
        int cutoff = prefixlen - p_i.prefix->size;
        int i;
        // Not exactly, just for comparison
        char *p_i_highkey = GetKeyDB2ByPtr(result, p_i.high);
        char *p_i_1_lowkey = GetKeyDB2ByPtr(result, p_i_1.low);
        for (i = p_i.low; i <= p_i.high; i++) {
            if (strncmp(GetKeyDB2ByPtr(result, i), p_i_highkey, cutoff) == 0) {
                // then key i can be move to the next prefix
                firstlow = i;
                break;
            }
        }
        for (; i <= p_i.high; i++) {
            // update the new prefix
            result->newoffset[i] += cutoff;
            result->newsize[i] -= cutoff;
            // result->memusage -= cutoff;
        }
        cutoff = prefixlen - p_i_1.prefix->size;
        for (int j = p_i_1.low; j <= p_i_1.high; j++) {
            // int cutoff = prefixlen - p_i_1.prefix->size;
            if (strncmp(GetKeyDB2ByPtr(result, j), p_i_1_lowkey, cutoff) == 0) {
                result->newoffset[j] += cutoff;
                result->newsize[j] -= cutoff;
                // result->memusage -= cutoff;
            }
            else {
                lasthigh = j - 1;
                break;
            }
        }
        // Last high has not been set, all keys in p[i + 1] share the given prefix
        if (lasthigh == -1) {
            lasthigh = p_i_1.high;
        }

        PrefixMetaData newmetadata = PrefixMetaData(leftlast, prefixlen, firstlow, lasthigh);

        if (firstlow == p_i.low && lasthigh == p_i_1.high) {
            // cover all keys in p[i] and p[i+1]
            result->prefixes[index] = newmetadata;
            result->prefixes.erase(result->prefixes.begin() + index + 1);
            return index + 1;
        }
        else if (firstlow > p_i.low && lasthigh == p_i_1.high) {
            // cover all keys in p[i+1]
            result->prefixes[index].high = firstlow - 1;
            result->prefixes[index + 1] = newmetadata;
            return index + 1;
        }
        else if (firstlow == p_i.low && lasthigh < p_i_1.high) {
            // cover all keys in p[i]
            result->prefixes[index] = newmetadata;
            result->prefixes[index + 1].low = lasthigh + 1;
            return index + 1;
        }
        else {
            // generate a new prefixmetadata
            result->prefixes[index].high = firstlow - 1;
            result->prefixes[index + 1].low = lasthigh + 1;
            result->prefixes.insert(result->prefixes.begin() + index + 1, newmetadata);
            return index + 2;
        }
    }
    delete leftlast;
    delete rightfirst;
    // Do nothing
    return index + 1;
}

prefixOptimization *prefix_expand(DB2Node *node) {
    int nextprefix = 0;
    prefixOptimization *result = new prefixOptimization();
    // vector<Key_c> keysCopy(keys);
    // result->memusage = node->memusage;
    result->memusage = 0;
    memcpy(result->base, node->base, sizeof(char) * MAX_SIZE_IN_BYTES);
    memcpy(result->newoffset, node->keys_offset, sizeof(uint16_t) * kNumberBound);
    memcpy(result->newsize, node->keys_size, sizeof(uint8_t) * kNumberBound);
    result->prefixes = node->prefixMetadata;

    // we only move the offset in expang_prefix, don't change the content of base
    while (nextprefix < result->prefixes.size() - 1) {
        nextprefix = expand_prefixes_in_boundary(result, nextprefix);
    }
    // result.keys = keysCopy;
    // result.prefixMetadatas = metadataCopy;
    // cout << "expand: key-" << result->memusage;
    int prefix_count = 0;
    for (auto pfx : result->prefixes) {
        result->memusage += pfx.prefix->size;
        prefix_count += pfx.prefix->size;
        for (int i = pfx.low; i <= pfx.high; i++)
            result->memusage += result->newsize[i];
    }
    // cout << "expand: pfx: " << prefix_count << ", total:" << result->memusage << endl;
    return result;
}

// // Find optimization with minimum cost to apply
// optimizationType find_prefix_optimization_to_apply(prefixOptimization prefixExpandResult,
//                                                    prefixOptimization prefixMergeResult) {
//     int prefixExpandCost = calculate_cost_of_optimization(prefixExpandResult);
//     int prefixMergeCost = calculate_cost_of_optimization(prefixMergeResult);
//     if (prefixMergeCost < prefixExpandCost) {
//         return prefixmerge;
//     }
//     return prefixexpand;
// }

void apply_prefix_optimization(DB2Node *node) {
    if (node->prefixMetadata.size() == 1) {
        // Only one common prefix, so prefix is the only prefix
        Data *prefix = node->prefixMetadata[0].prefix;
        vector<PrefixMetaData> newprefixmetadatas;
        vector<Key_c> newkeys;
        char *newbase = NewPage();
        uint8_t *newsize = new uint8_t[kNumberBound];
        uint16_t *newoffset = new uint16_t[kNumberBound];
        int memusage = 0;

        // string prevkey = nodeprefix + node->keys.at(0).value;
        // char *prevkey = new char[prefix->size + node->keys_size[0] + 1];
        // strcpy(prevkey, prefix->addr());
        // strcpy(prevkey + prefix->size, GetKey(node, 0));

        int prevprefix_len = 0;
        PrefixMetaData prefixmetadata = PrefixMetaData("", 0, 0, 0);
        bool uninitialized = true;
        // prevkey = nodeprefix + getkey
        // currkey = nodeprefix + getkey

        for (uint32_t i = 1; i < node->size; i++) {
            // string currkey = nodeprefix + node->keys.at(i).value;
            // string currprefix = get_common_prefix(currkey, prevkey);

            // the curprefix doesn't include the original prefix
            // int curkey_len = node->keys_size[i];
            int curprefix_len = get_common_prefix_len(GetKey(node, i), GetKey(node, i - 1),
                                                      node->keys_size[i], node->keys_size[i - 1]);
            if (uninitialized) {
                // Key_c key = node->keys.at(i - 1);
                // key.value = string_to_char(prevkey.substr(currprefix.length()));
                // newkeys.push_back(key);
                // key = node->keys.at(i);
                // key.value = string_to_char(currkey.substr(currprefix.length()));
                // newkeys.push_back(key);
                // Insert the suffix of i, i-1 to the new page

                WriteKeyDB2Page(newbase, memusage, i - 1, newsize, newoffset,
                                GetKey(node, i - 1), node->keys_size[i - 1], curprefix_len);
                WriteKeyDB2Page(newbase, memusage, i, newsize, newoffset,
                                GetKey(node, i), node->keys_size[i], curprefix_len);

                char *key_i = new char[prefix->size + node->keys_size[0] + 1];
                strcpy(key_i, prefix->addr());
                strcpy(key_i + prefix->size, GetKey(node, i));

                prefixmetadata.prefix = new Data(key_i, prefix->size + curprefix_len);
                prefixmetadata.high += 1;
                prevprefix_len = curprefix_len;
                uninitialized = false;

                delete key_i;
            }
            // TODO: After search is right, can change this to >=
            else if (curprefix_len == prevprefix_len) {
                // Add to the current prefix
                // Key_c key = node->keys.at(i);
                // key.value = string_to_char(currkey.substr(prevprefix.length()));
                // newkeys.push_back(key);

                WriteKeyDB2Page(newbase, memusage, i, newsize, newoffset,
                                GetKey(node, i), node->keys_size[i], curprefix_len);

                prefixmetadata.high += 1;
            }
            else {
                newprefixmetadatas.push_back(prefixmetadata);
                prefixmetadata = PrefixMetaData(prefix->addr(), prefix->size, i, i);
                uninitialized = true;
            }
            // prevkey = currkey;
        }
        if (uninitialized) {
            // The last key hasn't been added, and it doesn't have prefix
            // Key_c key = node->keys.at(node->keys.size() - 1);
            // key.value = string_to_char(prevkey);
            // newkeys.push_back(key);
            // newkeys.push_back(prevkey);
            WriteKeyDB2Page(newbase, memusage, node->size - 1, newsize, newoffset,
                            GetKey(node, node->size - 1), node->keys_size[node->size - 1], 0);
        }

        newprefixmetadatas.push_back(prefixmetadata);
        node->prefixMetadata = newprefixmetadatas;
        node->memusage = memusage;
        UpdateBase(node, newbase);
        UpdateOffset(node, newoffset);
        UpdateSize(node, newsize);
    }
    else {
        int ori = 0;
        for (int i = 0; i < node->size; i++)
            ori += node->keys_size[i];
        cout << "original: key-" << ori;
        for (auto pfx : node->prefixMetadata)
            ori += pfx.prefix->size;
        cout << "total-" << ori << endl;
        prefixOptimization *expand = prefix_expand(node);
        prefixOptimization *merge = prefix_merge(node);
        // optimizationType type = find_prefix_optimization_to_apply(prefixExpandResult, prefixMergeResult);
        cout << "expand: " << expand->memusage << ", merge: " << merge->memusage << endl;
        if (expand->memusage <= merge->memusage) {
            // Do prefix_expand
            cout << "expand" << endl;
            int newusage = 0;
            char *buf = NewPage();
            uint16_t *idx = new uint16_t[kNumberBound];
            for (int i = 0; i < node->size; i++) {
                strcpy(buf + newusage, expand->base + expand->newoffset[i]);
                idx[i] = newusage;
                newusage += expand->newsize[i] + 1;
            }
            node->memusage = newusage;
            UpdateBase(node, buf);
            UpdateOffset(node, idx);
            // CopyOffset(node, expand->newoffset);
            CopySize(node, expand->newsize);
            node->prefixMetadata = expand->prefixes;
        }
        else {
            // Do prefix_merge
            cout << "merge" << endl;
            int newusage = 0;
            char *buf = NewPage();
            uint8_t oldpfx_idx[node->size];
            // Track their old prefix_idx
            for (int i = 0; i < node->prefixMetadata.size(); i++) {
                auto pfx = node->prefixMetadata[i];
                for (int j = pfx.low; j <= pfx.high; j++)
                    oldpfx_idx[j] = i;
            }
            for (int i = 0; i < node->size; i++) {
                // Copy the shortened prefix to key
                auto oldpfx = node->prefixMetadata[oldpfx_idx[i]].prefix;
                auto extra_len = merge->newoffset[i];
                merge->newoffset[i] = newusage;
                strncpy(buf + newusage, oldpfx->addr() + oldpfx->size - extra_len, extra_len);
                strcpy(buf + newusage + extra_len, GetKey(node, i));
                newusage += merge->newsize[i] + 1;
            }
            node->memusage = newusage;
            UpdateBase(node, buf);
            CopyOffset(node, merge->newoffset);
            CopySize(node, merge->newsize);
            node->prefixMetadata = merge->prefixes;
        }

        delete expand;
        delete merge;
    }
}
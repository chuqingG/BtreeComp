#pragma once
#include "compression_db2.h"

closedRange find_closed_range(vector<PrefixMetaData> prefixmetadatas, string n_p_i, int p_i_pos) {
    int n_p_i_size = n_p_i.length();
    vector<PrefixMetaData> closedRangePrefixes;
    vector<int> positions;
    closedRange closedRange;
    if (n_p_i_size == 0) {
        return closedRange;
    }
    for (int i = p_i_pos - 1; i >= 0; i--) {
        string p_prev = prefixmetadatas.at(i).prefix;
        if (p_prev.compare(0, n_p_i_size, n_p_i) != 0) {
            break;
        }
        closedRangePrefixes.push_back(prefixmetadatas.at(i));
        positions.push_back(i);
    }
    // Reverse for ease of use
    reverse(closedRangePrefixes.begin(), closedRangePrefixes.end());
    reverse(positions.begin(), positions.end());

    closedRange.prefixMetadatas = closedRangePrefixes;
    closedRange.positions = positions;
    return closedRange;
}

int search_prefix_metadata(DB2Node *cursor, string_view key) {
    // binary search
    vector<PrefixMetaData> prefixMetadatas = cursor->prefixMetadata;
    int low = 0;
    int high = prefixMetadatas.size() - 1;
    int prefixcmp = -1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        PrefixMetaData midMetadata = prefixMetadatas.at(mid);
        string prefix = midMetadata.prefix;
        int prefixlen = prefix.length();
        prefixcmp = key.compare(0, prefixlen, prefix);
        int cmp = prefixcmp;
        if (prefixcmp == 0) {
            int prefixlow = midMetadata.low;
            int prefixhigh = midMetadata.high;
            int len = key.length() - prefixlen;
            int lowcmp = key.compare(prefixlen, len, cursor->keys.at(prefixlow).value);
            int highcmp = key.compare(prefixlen, len, cursor->keys.at(prefixhigh).value);
            cmp = (lowcmp >= 0) ^ (highcmp <= 0) ? lowcmp : 0;
        }
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }

    return -1;
}

int insert_prefix_metadata(DB2Node *cursor, string_view key) {
    vector<PrefixMetaData> prefixMetadatas = cursor->prefixMetadata;
    int low = 0;
    int high = prefixMetadatas.size() - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        PrefixMetaData midMetadata = prefixMetadatas.at(mid);
        string prefix = midMetadata.prefix;
        int prefixlen = prefix.length();
        int prefixcmp = key.compare(0, prefixlen, prefix);
        int cmp = prefixcmp;
        if (prefixcmp == 0) {
            int prefixlow = midMetadata.low;
            int prefixhigh = midMetadata.high;
            int len = key.length() - prefixlen;
            int lowcmp = key.compare(prefixlen, len, cursor->keys.at(prefixlow).value);
            int highcmp = key.compare(prefixlen, len, cursor->keys.at(prefixhigh).value);
            cmp = (lowcmp >= 0) ^ (highcmp <= 0) ? lowcmp : 0;
        }
        if (cmp == 0)
            return mid;
        else if (cmp > 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    int prevcmp = -1;
    if (high >= 0 && high < prefixMetadatas.size()) {
        string prevprefix = prefixMetadatas.at(high).prefix;
        prevcmp = key.compare(0, prevprefix.length(), prevprefix);
    }
    return prevcmp == 0 ? high : high + 1;
}

int find_insert_pos(DB2Node *node, const char *key, int (*insertfunc)(DB2Node *, const char *, int, int, bool &), bool &equal) {
    int metadatapos = insert_prefix_metadata(node, key);
    int insertpos;
    if (metadatapos == node->prefixMetadata.size()) {
        insertpos = node->size;
        PrefixMetaData newmetadata = PrefixMetaData("", insertpos, insertpos);
        node->prefixMetadata.insert(node->prefixMetadata.begin() + metadatapos, newmetadata);
    }
    else {
        PrefixMetaData metadata = node->prefixMetadata.at(metadatapos);
        string prefix = metadata.prefix;
        if (strncmp(key, prefix.data(), prefix.length())) {
            // key = key.substr(prefix.length());
            insertpos = insertfunc(node, key + prefix.length(), metadata.low, metadata.high, equal);
            if (!equal)
                node->prefixMetadata.at(metadatapos).high += 1;
        }
        else {
            insertpos = node->prefixMetadata.at(metadatapos).low;
            PrefixMetaData newmetadata = PrefixMetaData("", insertpos, insertpos);
            node->prefixMetadata.insert(node->prefixMetadata.begin() + metadatapos, newmetadata);
        }
        if (!equal) {
            for (uint32_t i = metadatapos + 1; i < node->prefixMetadata.size(); i++) {
                node->prefixMetadata.at(i).low += 1;
                node->prefixMetadata.at(i).high += 1;
            }
        }
    }

    return insertpos;
}

db2split perform_split(DB2Node *node, int split, bool isleaf) {
    vector<PrefixMetaData> metadatas = node->prefixMetadata;
    vector<PrefixMetaData> leftmetadatas;
    vector<PrefixMetaData> rightmetadatas;
    PrefixMetaData leftmetadata, rightmetadata;
    int rightindex = 0;
    string splitprefix;
    db2split result;
    for (uint32_t i = 0; i < metadatas.size(); i++) {
        if (split >= metadatas.at(i).low && split <= metadatas.at(i).high) {
            splitprefix = metadatas.at(i).prefix;

            if (split != metadatas.at(i).low) {
                leftmetadata = PrefixMetaData(metadatas.at(i).prefix, metadatas.at(i).low, split - 1);
                leftmetadatas.push_back(leftmetadata);
            }

            int rsize = isleaf ? metadatas.at(i).high - split : metadatas.at(i).high - (split + 1);
            if (rsize >= 0) {
                rightmetadata = PrefixMetaData(metadatas.at(i).prefix, rightindex, rightindex + rsize);
                rightindex = rightindex + rsize + 1;
                rightmetadatas.push_back(rightmetadata);
            }
        }
        else if (split <= metadatas.at(i).low) {
            int size = metadatas.at(i).high - metadatas.at(i).low;
            rightmetadata = PrefixMetaData(metadatas.at(i).prefix,
                                           rightindex, rightindex + size);
            rightindex = rightindex + size + 1;
            rightmetadatas.push_back(rightmetadata);
        }
        else {
            leftmetadata = metadatas.at(i);
            leftmetadatas.push_back(leftmetadata);
        }
    }
    result.leftmetadatas = leftmetadatas;
    result.rightmetadatas = rightmetadatas;
    result.splitprefix = splitprefix;
    return result;
}

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized
int calculate_prefix_merge_cost(vector<Key_c> keys, vector<PrefixMetaData> segment, string prefix) {
    int cost = prefix.length() * segment.size();
    for (uint32_t i = 0; i < segment.size(); i++) {
        int low = segment.at(i).low;
        int high = segment.at(i).high;
        for (int j = low; j <= high; j++) {
            string decompressed = segment.at(i).prefix + keys.at(j).value;
            string compressedkey = decompressed.substr(prefix.length());
            cost += compressedkey.length();
        }
    }
    return cost;
}

// Cost of optimization is calculated as size of prefixes + size of suffixes
int calculate_cost_of_optimization(prefixOptimization result) {
    int cost = 0;
    for (int i = 0; i < result.prefixMetadatas.size(); i++) {
        int low = result.prefixMetadatas.at(i).low;
        int high = result.prefixMetadatas.at(i).high;
        cost += result.prefixMetadatas.at(i).prefix.length();
        for (int j = low; j <= high; j++) {
            cost += result.keys.at(j).getSize();
        }
    }
    return cost;
}

prefixMergeSegment find_best_segment_of_size_k(vector<Key_c> keys, closedRange closedRange, int k) {
    prefixMergeSegment result;
    vector<PrefixMetaData> bestsegment;
    vector<PrefixMetaData> currsegment;
    string bestprefix = "";
    string currprefix = "";
    int bestcost = INT32_MAX;
    int currcost = 0;
    int firstindex = 0;
    int bestindex = 0;
    for (int i = 0; i < closedRange.prefixMetadatas.size(); i++) {
        if (i % k == 0) {
            currcost = calculate_prefix_merge_cost(keys, currsegment, currprefix);
            firstindex = closedRange.positions.at(i);
            if (i != 0 && currcost < bestcost) {
                bestprefix = currprefix;
                bestsegment = currsegment;
                bestcost = currcost;
                bestindex = firstindex;
            }
            currprefix = closedRange.prefixMetadatas.at(i).prefix;
            currsegment.clear();
        }
        else {
            currprefix = get_common_prefix(currprefix, closedRange.prefixMetadatas.at(i).prefix);
        }
        currsegment.push_back(closedRange.prefixMetadatas.at(i));
    }
    if (currcost < bestcost) {
        bestprefix = currprefix;
        bestsegment = currsegment;
        bestcost = currcost;
        bestindex = firstindex;
    }
    result.segment = bestsegment;
    result.prefix = bestprefix;
    result.cost = bestcost;
    result.firstindex = bestindex;
    return result;
}

prefixMergeSegment find_best_segment_in_closed_range(vector<Key_c> keys, closedRange closedRange) {
    prefixMergeSegment bestsegment;
    int bestcost = INT32_MAX;
    // Start searching for segments of size 2
    for (int i = 2; i <= closedRange.prefixMetadatas.size(); i++) {
        prefixMergeSegment segment = find_best_segment_of_size_k(keys, closedRange, i);
        if (segment.cost < bestcost) {
            bestcost = segment.cost;
            bestsegment = segment;
        }
    }
    return bestsegment;
}

void merge_prefixes_in_segment(vector<Key_c> &keys, vector<PrefixMetaData> &prefixmetadatas,
                               prefixMergeSegment bestsegment, string newprefix) {
    // cout << "Best segment prefix " << bestsegment.prefix << endl;
    // cout << "Best segment first index " << bestsegment.firstindex << endl;
    vector<PrefixMetaData> segment = bestsegment.segment;
    int firstpos = bestsegment.firstindex;
    int firstlow = segment.at(0).low;
    int lasthigh = 0;
    for (uint32_t i = 0; i < segment.size(); i++) {
        string prevprefix = segment.at(i).prefix;
        int low = segment.at(i).low;
        int high = segment.at(i).high;
        lasthigh = high;
        for (int j = low; j <= high; j++) {
            string decompressed = prevprefix + keys.at(j).value;
            // Representing null prefix
            if (newprefix.compare("") == 0) {
                keys.at(j).value = string_to_char(decompressed);
            }
            else {
                keys.at(j).value = string_to_char(decompressed.substr(newprefix.length()));
            }
        }
    }
    prefixmetadatas.erase(next(prefixmetadatas.begin(), firstpos), next(prefixmetadatas.begin(), firstpos + segment.size()));
    PrefixMetaData newmetadata = PrefixMetaData(newprefix, firstlow, lasthigh);
    prefixmetadatas.insert(prefixmetadatas.begin() + firstpos, newmetadata);
}

prefixOptimization prefix_merge(vector<Key_c> keys, vector<PrefixMetaData> prefixmetadatas) {
    prefixOptimization result;
    vector<Key_c> keysCopy(keys);
    vector<PrefixMetaData> metadataCopy(prefixmetadatas);
    int nextindex = 1;
    int numremoved = 0;
    while (nextindex < metadataCopy.size()) {
        numremoved = 0;
        string p_i = metadataCopy.at(nextindex).prefix;
        string p_i_minus_1 = metadataCopy.at(nextindex - 1).prefix;
        string p_i_first_key = p_i + keysCopy.at(metadataCopy.at(nextindex).low).value;
        string p_i_prev_last_key = p_i_minus_1 + keysCopy.at(metadataCopy.at(nextindex - 1).high).value;
        string n_p_i = get_common_prefix(p_i_first_key, p_i_prev_last_key);
        closedRange closedRange = find_closed_range(metadataCopy, n_p_i, nextindex);
        if (closedRange.prefixMetadatas.size() > 1) {
            if (n_p_i.length() == 0) {
                prefixMergeSegment bestSegment = find_best_segment_in_closed_range(keysCopy, closedRange);
                merge_prefixes_in_segment(keysCopy, metadataCopy, bestSegment, "");
                numremoved = bestSegment.segment.size();
            }
            else if (n_p_i.length() < p_i_minus_1.length()) {
                prefixMergeSegment bestSegment = find_best_segment_in_closed_range(keysCopy, closedRange);
                merge_prefixes_in_segment(keysCopy, metadataCopy, bestSegment, bestSegment.prefix);
                numremoved = bestSegment.segment.size();
            }
        }
        nextindex = nextindex + 1 - numremoved;
    }
    result.keys = keysCopy;
    result.prefixMetadatas = metadataCopy;
    return result;
}

int expand_prefixes_in_boundary(vector<Key_c> &keys, vector<PrefixMetaData> &prefixmetadatas, int index) {
    PrefixMetaData p_i = prefixmetadatas.at(index);
    PrefixMetaData p_i_plus_1 = prefixmetadatas.at(index + 1);
    string common_prefix = get_common_prefix(p_i.prefix + keys.at(p_i.high).value,
                                             p_i_plus_1.prefix + keys.at(p_i_plus_1.low).value);
    int prefixlen = common_prefix.length();
    int firstlow = -1;
    int lasthigh = -1;
    if (prefixlen > 0 && prefixlen >= max(p_i.prefix.length(), p_i_plus_1.prefix.length())) {
        for (int i = p_i.low; i <= p_i.high; i++) {
            string decompressed = p_i.prefix + keys.at(i).value;
            if (decompressed.compare(0, prefixlen, common_prefix) == 0) {
                // First low has not been set
                if (firstlow == -1)
                    firstlow = i;
                keys.at(i).value = string_to_char(decompressed.substr(prefixlen));
            }
        }

        for (int i = p_i_plus_1.low; i <= p_i_plus_1.high; i++) {
            string decompressed = p_i_plus_1.prefix + keys.at(i).value;
            if (decompressed.compare(0, prefixlen, common_prefix) == 0) {
                keys.at(i).value = string_to_char(decompressed.substr(prefixlen));
                lasthigh = i;
            }
            else {
                break;
            }
        }

        // Last high has not been set
        if (lasthigh == -1) {
            lasthigh = p_i_plus_1.high;
        }

        PrefixMetaData newmetadata = PrefixMetaData(common_prefix, firstlow, lasthigh);

        if (firstlow == p_i.low && lasthigh == p_i_plus_1.high) {
            prefixmetadatas.at(index) = newmetadata;
            prefixmetadatas.erase(prefixmetadatas.begin() + index + 1);
            return index + 1;
        }
        else if (firstlow > p_i.low && lasthigh == p_i_plus_1.high) {
            prefixmetadatas.at(index).high = firstlow - 1;
            prefixmetadatas.at(index + 1) = newmetadata;
            return index + 1;
        }
        else if (firstlow == p_i.low && lasthigh < p_i_plus_1.high) {
            prefixmetadatas.at(index) = newmetadata;
            prefixmetadatas.at(index + 1).low = lasthigh + 1;
            return index + 1;
        }
        else {
            prefixmetadatas.at(index).high = firstlow - 1;
            prefixmetadatas.at(index + 1).low = lasthigh + 1;
            prefixmetadatas.insert(prefixmetadatas.begin() + index + 1, newmetadata);
            return index + 2;
        }
    }
    return index + 1;
}

prefixOptimization prefix_expand(vector<Key_c> keys, vector<PrefixMetaData> prefixmetadatas) {
    int nextprefix = 0;
    prefixOptimization result;
    vector<Key_c> keysCopy(keys);
    vector<PrefixMetaData> metadataCopy(prefixmetadatas);

    while (nextprefix < metadataCopy.size() - 1) {
        nextprefix = expand_prefixes_in_boundary(keysCopy, metadataCopy, nextprefix);
    }
    result.keys = keysCopy;
    result.prefixMetadatas = metadataCopy;
    return result;
}

// Find optimization with minimum cost to apply
optimizationType find_prefix_optimization_to_apply(prefixOptimization prefixExpandResult,
                                                   prefixOptimization prefixMergeResult) {
    int prefixExpandCost = calculate_cost_of_optimization(prefixExpandResult);
    int prefixMergeCost = calculate_cost_of_optimization(prefixMergeResult);
    if (prefixMergeCost < prefixExpandCost) {
        return prefixmerge;
    }
    return prefixexpand;
}

void apply_prefix_optimization(DB2Node *node) {
    if (node->prefixMetadata.size() == 1) {
        string nodeprefix = node->prefixMetadata.at(0).prefix;
        vector<PrefixMetaData> newprefixmetadatas;
        vector<Key_c> newkeys;
        string prev_key = "";
        string prevprefix = "";
        string prevkey = nodeprefix + node->keys.at(0).value;
        PrefixMetaData prefixmetadata = PrefixMetaData("", 0, 0);
        bool uninitialized = true;
        for (uint32_t i = 1; i < node->keys.size(); i++) {
            string currkey = nodeprefix + node->keys.at(i).value;
            string currprefix = get_common_prefix(currkey, prevkey);
            if (uninitialized) {
                Key_c key = node->keys.at(i - 1);
                key.value = string_to_char(prevkey.substr(currprefix.length()));
                newkeys.push_back(key);
                key = node->keys.at(i);
                key.value = string_to_char(currkey.substr(currprefix.length()));
                newkeys.push_back(key);
                prefixmetadata.prefix = currprefix;
                prefixmetadata.high += 1;
                prevprefix = currprefix;
                uninitialized = false;
            }
            else if (currprefix.compare(prevprefix) == 0) {
                Key_c key = node->keys.at(i);
                key.value = string_to_char(currkey.substr(prevprefix.length()));
                newkeys.push_back(key);
                prefixmetadata.high += 1;
            }
            else {
                newprefixmetadatas.push_back(prefixmetadata);
                prefixmetadata = PrefixMetaData("", i, i);
                uninitialized = true;
            }
            prevkey = currkey;
        }
        if (uninitialized) {
            Key_c key = node->keys.at(node->keys.size() - 1);
            key.value = string_to_char(prevkey);
            newkeys.push_back(key);
            // newkeys.push_back(prevkey);
        }
        newprefixmetadatas.push_back(prefixmetadata);
        node->keys = newkeys;
        node->prefixMetadata = newprefixmetadatas;
    }
    else {
        prefixOptimization prefixExpandResult = prefix_expand(node->keys, node->prefixMetadata);
        prefixOptimization prefixMergeResult = prefix_merge(node->keys, node->prefixMetadata);
        optimizationType type = find_prefix_optimization_to_apply(prefixExpandResult, prefixMergeResult);
        if (type == prefixexpand) {
            node->keys = prefixExpandResult.keys;
            node->prefixMetadata = prefixExpandResult.prefixMetadatas;
        }
        else {
            node->keys = prefixMergeResult.keys;
            node->prefixMetadata = prefixMergeResult.prefixMetadatas;
        }
    }
}
#pragma once
#include "compression_db2.h"

prefixOptimization::prefixOptimization() {
    space_top = 0;
    pfx_top = 0;
    pfx_size = 0;
    base = NewPageDB2();
    SetEmptyPageDB2(base);
}

prefixOptimization::~prefixOptimization() {
    delete[] base;
}

closedRange find_closed_range(prefixOptimization *result,
                              const char *newprefix, int prefixlen, int p_i_pos) {
    closedRange closedRange;
    if (prefixlen == 0) {
        return closedRange;
    }
    int i;
    for (i = p_i_pos - 1; i >= 0; i--) {
        DB2pfxhead *h_i = GetPfxInPageDB2(result, i);
        if (strncmp(GetPfxDB2(result, h_i->pfx_offset), newprefix, prefixlen) != 0)
            break;
    }
    i++;
    closedRange.low = i;

    closedRange.high = p_i_pos - 1;
    return closedRange;
}

int find_prefix_pos(NodeDB2 *cursor, const char *key, int keylen, bool for_insert_or_nonleaf) {
    int low = 0;
    int high = cursor->pfx_size - 1;
    while (low <= high) {
        int cur = low + (high - low) / 2;
        DB2pfxhead *pfx = GetHeaderDB2pfx(cursor, cur);

        int prefixcmp = strncmp(key, PfxOffset(cursor, pfx->pfx_offset), pfx->pfx_len);
        int cmp = prefixcmp;
        if (prefixcmp == 0) {
            DB2head *h_low = GetHeaderDB2(cursor, pfx->low);
            DB2head *h_high = GetHeaderDB2(cursor, pfx->high);

            int cut_keylen = keylen - pfx->pfx_len;
            const char *key_cutoff = key + pfx->pfx_len;
            int lowcmp = memcmp(key_cutoff, PageOffset(cursor, h_low->key_offset), sizeof(char) * cut_keylen);
            int highcmp = memcmp(key_cutoff, PageOffset(cursor, h_high->key_offset), sizeof(char) * cut_keylen);
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
    if (high >= 0 && high < cursor->pfx_size) {
        DB2pfxhead *prev_pfx = GetHeaderDB2pfx(cursor, high);
        prevcmp = memcmp(key, PfxOffset(cursor, prev_pfx->pfx_offset), sizeof(char) * prev_pfx->pfx_len);
    }
    return prevcmp == 0 ? high : high + 1;
}

// Cost function is defined based on space requirements for prefix + suffixes
// Cost must be minimized
// No, should be the space saved, and find a maximum
int calculate_prefix_merge_save(prefixOptimization *result, prefixMergeSegment *seg, Item *prefix) {
    // 1. first save the prefix header
    int save = sizeof(DB2pfxhead) * (seg->segment.high - seg->segment.low);

    // 2. for each prefix, subtract the extra prefix slot added
    for (int i = seg->segment.low; i <= seg->segment.high; i++) {
        DB2pfxhead *h_i = GetPfxInPageDB2(result, i);
        int extra_len = h_i->pfx_len - prefix->size;
        save -= extra_len * (h_i->high - h_i->low + 1);
    }
    return save;
}

prefixMergeSegment find_best_segment_of_size_k(prefixOptimization *result, closedRange *range, int k) {
    prefixMergeSegment seg;
    closedRange curr_range;

    int currsave = 0;
    int firstindex = 0;
    Item currprefix;

    for (int i = range->low; i <= range->high; i++) {
        DB2pfxhead *p_i = GetPfxInPageDB2(result, i);
        int idx = i - range->low;
        if (idx % k == 0) {
            currsave = calculate_prefix_merge_save(result, &seg, &currprefix);
            firstindex = i;
            if (idx != 0 && currsave > seg.save) {
                seg.prefix = currprefix;
                seg.segment = curr_range;
                seg.save = currsave;
                seg.firstindex = firstindex;
            }
            currprefix.addr = GetPfxDB2(result, p_i->pfx_offset);
            currprefix.size = p_i->pfx_len;

            curr_range.low = i;
            curr_range.high = i - 1;
        }
        else {
            // Only need to update the length?
            currprefix.size = get_common_prefix_len(currprefix.addr, GetPfxDB2(result, p_i->pfx_offset),
                                                    currprefix.size, p_i->pfx_len);
        }
        curr_range.high++;
    }

    if (currsave > seg.save) {
        seg.prefix = currprefix;
        seg.segment = curr_range;
        seg.save = currsave;
        seg.firstindex = firstindex;
    }

    return seg;
}

prefixMergeSegment find_best_segment_in_closed_range(prefixOptimization *result, closedRange *closedRange) {
    prefixMergeSegment bestsegment;
    int max_save = INT32_MIN;
    // Start searching for segments of size
    int rangesize = closedRange->high - closedRange->low + 1;
    for (int i = 2; i <= rangesize; i++) {
        prefixMergeSegment segment = find_best_segment_of_size_k(result, closedRange, i);
        if (segment.save > max_save) {
            max_save = segment.save;
            bestsegment = segment;
        }
    }
    return bestsegment;
}

void merge_prefixes_in_segment(prefixOptimization *result,
                               prefixMergeSegment *bestsegment) {
    // the result.prefix should be shorten in this section

    // firstpos is the same as segment.low
    // int firstpos = bestsegment->segment.low;
    int firstlow = -1;
    int lasthigh = 0;
    for (int i = bestsegment->segment.low; i <= bestsegment->segment.high; i++) {
        DB2pfxhead *p_i = GetPfxInPageDB2(result, i);
        if (firstlow < 0)
            firstlow = p_i->low;
        lasthigh = p_i->high;
        for (int j = p_i->low; j <= p_i->high; j++) {
            DB2head *h_j = GetHeaderInPageDB2(result, j);
            int extra_len = p_i->pfx_len - bestsegment->prefix.size;
            h_j->key_len += extra_len;
            h_j->key_offset = 0 - extra_len;
        }
    }

    int dist = bestsegment->segment.high - bestsegment->segment.low;
    for (int i = bestsegment->segment.high + 1; i < result->pfx_size; i++) {
        memcpy(GetPfxInPageDB2(result, i - dist), GetPfxInPageDB2(result, i), sizeof(DB2pfxhead));
    }
    result->pfx_size -= dist;
    DB2pfxhead *newhead = GetPfxInPageDB2(result, bestsegment->segment.low);
    // no need to construct new prefix string for that, can simply concat from p[low]
    char *pfx = GetPfxDB2(result, newhead->pfx_offset);
    pfx[bestsegment->prefix.size] = '\0';
    newhead->pfx_len = bestsegment->prefix.size;
    newhead->low = firstlow;
    newhead->high = lasthigh;
    return;
}

prefixOptimization *prefix_merge(NodeDB2 *node) {
    prefixOptimization *result = new prefixOptimization();

    memcpy(result->base, node->base, sizeof(char) * (MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE));
    // memcpy(result->pfxbase, node->pfxbase, sizeof(char) * DB2_PFX_MAX_SIZE);
    result->pfx_size = node->pfx_size;

    int nextindex = 1;
    int numremoved;
    while (nextindex < result->pfx_size) {
        DB2pfxhead *p_i = GetPfxInPageDB2(result, nextindex);
        DB2pfxhead *p_prev = GetPfxInPageDB2(result, nextindex - 1);

        // DB2head *head_ll = GetHeaderInPageDB2(result, p_prev->high);
        // DB2head *head_rf = GetHeaderInPageDB2(result, p_i->low);

        // we don't need to decompress the string,
        // Because the common prefix should be shorter than any existing prefixes

        int prefixlen = get_common_prefix_len(GetPfxDB2(result, p_prev->pfx_offset),
                                              GetPfxDB2(result, p_i->pfx_offset),
                                              p_prev->pfx_len, p_i->pfx_len);
        // TODO: looks like don't need to care the suffix parts
        // the range is closed like [low, high], within the scope of [0:nextindex)
        closedRange closedRange = find_closed_range(result, GetPfxDB2(result, p_i->pfx_offset),
                                                    prefixlen, nextindex);

        numremoved = 0;
        if (closedRange.high - closedRange.low > 0) {
            // sizeof closedrange > 1
            if (prefixlen < p_prev->pfx_len) {
                prefixMergeSegment bestSegment = find_best_segment_in_closed_range(result, &closedRange);
                merge_prefixes_in_segment(result, &bestSegment);
                numremoved = bestSegment.segment.high - bestSegment.segment.low + 1;
            }
        }
        nextindex = nextindex + 1 - numremoved;
    }

    for (int i = 0; i < result->pfx_size; i++) {
        DB2pfxhead *head_i = GetPfxInPageDB2(result, i);
        result->pfx_top += head_i->pfx_len + 1;
        for (int j = head_i->low; j < head_i->high; j++) {
            DB2head *h_j = GetHeaderInPageDB2(result, j);
            result->space_top += h_j->key_len + 1;
        }
    }

    return result;
}

int expand_prefixes_in_boundary(prefixOptimization *result, int index) {
    // update prefixmetadata and keys in the result
    DB2pfxhead *pfx_i = GetPfxInPageDB2(result, index);
    DB2pfxhead *pfx_i_1 = GetPfxInPageDB2(result, index + 1);

    DB2head *head_ll = GetHeaderInPageDB2(result, pfx_i->high);
    DB2head *head_rf = GetHeaderInPageDB2(result, pfx_i_1->low);

    int ll_len = pfx_i->pfx_len + head_ll->key_len;
    int rf_len = pfx_i_1->pfx_len + head_rf->key_len;

    char *leftlast = new char[ll_len + 1];
    char *rightfirst = new char[rf_len + 1];
    strcpy(leftlast, GetPfxDB2(result, pfx_i->pfx_offset));
    strcpy(leftlast + pfx_i->pfx_len, GetKeyDB2(result, head_ll->key_offset));
    strcpy(rightfirst, GetPfxDB2(result, pfx_i_1->pfx_offset));
    strcpy(rightfirst + pfx_i_1->pfx_len, GetKeyDB2(result, head_rf->key_offset));

    int prefixlen = get_common_prefix_len(leftlast, rightfirst, ll_len, rf_len);

    int firstlow = -1;
    int lasthigh = -1;

    if (prefixlen > 0 && prefixlen >= max(pfx_i->pfx_len, pfx_i_1->pfx_len)) {
        int cutoff = prefixlen - pfx_i->pfx_len;
        int i;
        // Not exactly, just for comparison
        char *p_i_highkey = GetKeyDB2(result, head_ll->key_offset);
        char *p_i_1_lowkey = GetKeyDB2(result, head_rf->key_offset);
        for (i = pfx_i->low; i <= pfx_i->high; i++) {
            DB2head *head_i = GetHeaderInPageDB2(result, i);
            if (strncmp(GetKeyDB2(result, head_i->key_offset), p_i_highkey, cutoff) == 0) {
                // then key i can be move to the next prefix
                firstlow = i;
                break;
            }
        }
        for (; i <= pfx_i->high; i++) {
            // update the new prefix
            DB2head *head_i = GetHeaderInPageDB2(result, i);
            head_i->key_offset += cutoff;
            head_i->key_len -= cutoff;
        }
        cutoff = prefixlen - pfx_i_1->pfx_len;
        for (int j = pfx_i_1->low; j <= pfx_i_1->high; j++) {
            DB2head *head_j = GetHeaderInPageDB2(result, j);
            if (strncmp(GetKeyDB2(result, head_j->key_offset), p_i_1_lowkey, cutoff) == 0) {
                head_j->key_offset += cutoff;
                head_j->key_len -= cutoff;
            }
            else {
                lasthigh = j - 1;
                break;
            }
        }
        // Last high has not been set, all keys in p[i + 1] share the given prefix
        if (lasthigh == -1) {
            lasthigh = pfx_i_1->high;
        }

        prefixItem pfxitem;

        pfxitem.prefix.addr = leftlast;
        pfxitem.prefix.size = prefixlen;
        pfxitem.low = firstlow;
        pfxitem.high = lasthigh;

        if (firstlow == pfx_i->low && lasthigh == pfx_i_1->high) {
            // cover all keys in p[i] and p[i+1]
            // update pfx_head[i]
            strncpy(result->base + MAX_SIZE_IN_BYTES + result->pfx_top, pfxitem.prefix.addr, pfxitem.prefix.size);
            (result->base + MAX_SIZE_IN_BYTES + result->pfx_top)[pfxitem.prefix.size] = '\0';
            pfx_i->pfx_offset = result->pfx_top;
            pfx_i->pfx_len = pfxitem.prefix.size;
            pfx_i->low = pfxitem.low;
            pfx_i->high = pfxitem.high;
            result->pfx_top += pfxitem.prefix.size + 1;

            // remove pfx_head[i+1]
            for (int i = index + 2; i < result->pfx_size; i++) {
                memcpy(GetPfxInPageDB2(result, i - 1), GetPfxInPageDB2(result, i), sizeof(DB2pfxhead));
            }
            result->pfx_size--;
            return index + 1;
        }
        else if (firstlow > pfx_i->low && lasthigh == pfx_i_1->high) {
            // cover all keys in p[i+1]
            pfx_i->high = firstlow - 1;
            // update pfx_head[i]
            strncpy(result->base + MAX_SIZE_IN_BYTES + result->pfx_top, pfxitem.prefix.addr, pfxitem.prefix.size);
            (result->base + MAX_SIZE_IN_BYTES + result->pfx_top)[pfxitem.prefix.size] = '\0';
            pfx_i_1->pfx_offset = result->pfx_top;
            pfx_i_1->pfx_len = pfxitem.prefix.size;
            pfx_i_1->low = pfxitem.low;
            pfx_i_1->high = pfxitem.high;
            result->pfx_top += pfxitem.prefix.size + 1;
            return index + 1;
        }
        else if (firstlow == pfx_i->low && lasthigh < pfx_i_1->high) {
            // cover all keys in p[i]
            // update pfx_head[i]
            strncpy(result->base + MAX_SIZE_IN_BYTES + result->pfx_top, pfxitem.prefix.addr, pfxitem.prefix.size);
            (result->base + MAX_SIZE_IN_BYTES + result->pfx_top)[pfxitem.prefix.size] = '\0';
            pfx_i->pfx_offset = result->pfx_top;
            pfx_i->pfx_len = pfxitem.prefix.size;
            pfx_i->low = pfxitem.low;
            pfx_i->high = pfxitem.high;
            result->pfx_top += pfxitem.prefix.size + 1;
            pfx_i_1->low = lasthigh + 1;
            return index + 1;
        }
        else {
            // generate a new prefixmetadata
            pfx_i->high = firstlow - 1;
            pfx_i_1->low = lasthigh + 1;
            // shift header
            for (int i = result->pfx_size; i > index + 1; i--) {
                memcpy(GetPfxInPageDB2(result, i), GetPfxInPageDB2(result, i - 1), sizeof(DB2pfxhead));
            }
            strncpy(result->base + MAX_SIZE_IN_BYTES + result->pfx_top, pfxitem.prefix.addr, pfxitem.prefix.size);
            (result->base + MAX_SIZE_IN_BYTES + result->pfx_top)[pfxitem.prefix.size] = '\0';
            pfx_i_1->pfx_offset = result->pfx_top;
            pfx_i_1->pfx_len = pfxitem.prefix.size;
            pfx_i_1->low = pfxitem.low;
            pfx_i_1->high = pfxitem.high;
            result->pfx_top += pfxitem.prefix.size + 1;
            result->pfx_size++;
            return index + 2;
        }
    }
    delete[] leftlast;
    delete[] rightfirst;
    // Do nothing
    return index + 1;
}

prefixOptimization *prefix_expand(NodeDB2 *node) {
    int nextprefix = 0;
    prefixOptimization *result = new prefixOptimization();

    memcpy(result->base, node->base, sizeof(char) * (MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE));
    // memcpy(result->pfxbase, node->pfxbase, sizeof(char) * DB2_PFX_MAX_SIZE);
    result->pfx_size = node->pfx_size;
    // copy pfx_top, because the prefix can be extended in prefix expand
    result->pfx_top = node->pfx_top;

    // we only move the offset in expang_prefix, don't change the content of base
    while (nextprefix < result->pfx_size - 1) {
        nextprefix = expand_prefixes_in_boundary(result, nextprefix);
    }

    result->pfx_top = 0;
    for (int i = 0; i < result->pfx_size; i++) {
        DB2pfxhead *head_i = GetPfxInPageDB2(result, i);
        result->pfx_top += head_i->pfx_len + 1;
        for (int j = head_i->low; j < head_i->high; j++) {
            DB2head *h_j = GetHeaderInPageDB2(result, j);
            result->space_top += h_j->key_len + 1;
        }
    }

    return result;
}

void apply_prefix_optimization(NodeDB2 *node) {
    if (node->pfx_size == 1) {
        // Only one common prefix, so prefix is the only prefix
        DB2pfxhead *pfxhead = GetHeaderDB2pfx(node, 0);

        // vector<Key_c> newkeys;
        char *newbase = NewPageDB2();
        SetEmptyPageDB2(newbase);
        int memusage = 0, pfx_top = 0;
        int pfx_size = 0;

        prefixItem pfxitem;

        int prevprefix_len = 0;
        bool uninitialized = true;
        int i;
        for (i = 1; i < node->size; i++) {
            // the curprefix doesn't include the original prefix
            DB2head *head_i = GetHeaderDB2(node, i);
            DB2head *head_prev = GetHeaderDB2(node, i - 1);
            int curprefix_len = get_common_prefix_len(PageOffset(node, head_i->key_offset), PageOffset(node, head_prev->key_offset),
                                                      head_i->key_len, head_prev->key_len);
            if (uninitialized) {
                // Insert the suffix of i, i-1 to the new page

                WriteKeyDB2Page(newbase, memusage, i - 1,
                                PageOffset(node, head_prev->key_offset),
                                head_prev->key_len, curprefix_len);
                WriteKeyDB2Page(newbase, memusage, i,
                                PageOffset(node, head_i->key_offset),
                                head_i->key_len, curprefix_len);

                int pfx_len = pfxhead->pfx_len + curprefix_len;
                char *key_i = new char[pfx_len + 1];
                strcpy(key_i, PfxOffset(node, pfxhead->pfx_offset));
                strncpy(key_i + pfxhead->pfx_len, PageOffset(node, head_i->key_offset), curprefix_len);
                key_i[pfx_len] = '\0';
                pfxitem.prefix.addr = key_i;
                pfxitem.prefix.size = pfx_len;
                pfxitem.prefix.newallocated = true;
                pfxitem.high++;

                prevprefix_len = curprefix_len;
                uninitialized = false;
            }
            // TODO: After search is right, can change this to >=
            // This cannot be simply change to >= and cut prevprefix_len , may optimize later
            else if (curprefix_len >= prevprefix_len) {
                // Add to the current prefix

                WriteKeyDB2Page(newbase, memusage, i,
                                PageOffset(node, head_i->key_offset),
                                head_i->key_len, prevprefix_len);
                pfxitem.high++;
            }
            else {
                WritePfxDB2Page(newbase, pfx_top, pfxitem, pfx_size);

                pfxitem.low = i;
                pfxitem.high = i;
                uninitialized = true;
                if (pfx_top + node->pfx_top + (2 + pfx_size) * sizeof(DB2pfxhead) >= DB2_PFX_MAX_SIZE - SPLIT_LIMIT) {
                    break;
                }
            }
        }
        if (uninitialized) {
            // The last key hasn't been added, and it doesn't have prefix
            // Or the prefix space is not enough, simply put these keys : [i,node.size)
            pfxitem.prefix.addr = PfxOffset(node, pfxhead->pfx_offset);
            pfxitem.prefix.size = pfxhead->pfx_len;
            pfxitem.prefix.newallocated = false;
            pfxitem.high = node->size - 1;
            for (int j = pfxitem.low; j < node->size; j++) {
                DB2head *head_j = GetHeaderDB2(node, j);
                WriteKeyDB2Page(newbase, memusage, j,
                                PageOffset(node, head_j->key_offset), head_j->key_len, 0);
            }
        }
        WritePfxDB2Page(newbase, pfx_top, pfxitem, pfx_size);

        node->space_top = memusage;
        node->pfx_top = pfx_top;
        node->pfx_size = pfx_size;

        UpdateBase(node, newbase);
    }
    else {
        prefixOptimization *expand = prefix_expand(node);
        prefixOptimization *merge = prefix_merge(node);
        if (expand->space_top <= merge->space_top) {
            // Do prefix_expand

            int new_top = 0, new_pfx_top = 0;
            char *buf = NewPageDB2();
            SetEmptyPageDB2(buf);

            // Update the key metadata
            for (int i = 0; i < node->size; i++) {
                DB2head *h_i = GetHeaderInPageDB2(expand, i);
                DB2head *newhead = (DB2head *)(buf + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(DB2head));
                strcpy(buf + new_top, GetKeyDB2(expand, h_i->key_offset));
                newhead->key_offset = new_top;
                newhead->key_len = h_i->key_len;
                new_top += h_i->key_len + 1;
            }
            // Update the prefix metadata
            for (int i = 0; i < expand->pfx_size; i++) {
                DB2pfxhead *p_i = GetPfxInPageDB2(expand, i);
                DB2pfxhead *newhead = (DB2pfxhead *)(buf + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE - (i + 1) * sizeof(DB2pfxhead));
                strcpy(buf + MAX_SIZE_IN_BYTES + new_pfx_top, GetPfxDB2(expand, p_i->pfx_offset));
                newhead->low = p_i->low;
                newhead->high = p_i->high;
                newhead->pfx_len = p_i->pfx_len;
                newhead->pfx_offset = new_pfx_top;
                new_pfx_top += p_i->pfx_len + 1;
            }
            node->pfx_top = new_pfx_top;
            node->pfx_size = expand->pfx_size;
            node->space_top = new_top;
            UpdateBase(node, buf);
            // UpdatePfx(node, newpfx);
        }
        else {
            // Do prefix_merge
            int new_top = 0, new_pfx_top = 0;
            char *buf = NewPageDB2();
            SetEmptyPageDB2(buf);

            uint8_t oldpfx_idx[node->size];
            // Track their old prefix_idx
            for (int i = 0; i < node->pfx_size; i++) {
                DB2pfxhead *pfx = GetHeaderDB2pfx(node, i);
                for (int j = pfx->low; j <= pfx->high; j++)
                    oldpfx_idx[j] = i;
            }
            for (int i = 0; i < node->size; i++) {
                // Copy the shortened prefix to key
                DB2pfxhead *oldpfx = GetHeaderDB2pfx(node, oldpfx_idx[i]);
                DB2head *old_i = GetHeaderDB2(node, i);
                DB2head *merge_i = GetHeaderInPageDB2(merge, i);
                DB2head *newhead = (DB2head *)(buf + MAX_SIZE_IN_BYTES - (i + 1) * sizeof(DB2head));
                // a negative offset means extra length
                int extra_len = merge_i->key_offset < 0 ? (-merge_i->key_offset) : 0;
                newhead->key_offset = new_top;
                newhead->key_len = merge_i->key_len;
                strncpy(buf + new_top, PfxOffset(node, oldpfx->pfx_offset) + oldpfx->pfx_len - extra_len, extra_len);
                strcpy(buf + new_top + extra_len, PageOffset(node, old_i->key_offset));
                new_top += merge_i->key_len + 1;
            }
            // Update the prefix metadata
            for (int i = 0; i < merge->pfx_size; i++) {
                DB2pfxhead *p_i = GetPfxInPageDB2(merge, i);
                DB2pfxhead *newhead = (DB2pfxhead *)(buf + MAX_SIZE_IN_BYTES + DB2_PFX_MAX_SIZE - (i + 1) * sizeof(DB2pfxhead));
                strcpy(buf + MAX_SIZE_IN_BYTES + new_pfx_top, GetPfxDB2(expand, p_i->pfx_offset));
                memcpy(newhead, p_i, sizeof(DB2pfxhead));
                newhead->pfx_offset = new_pfx_top;
                new_pfx_top += p_i->pfx_len + 1;
            }

            node->space_top = new_top;
            UpdateBase(node, buf);
            node->pfx_size = merge->pfx_size;
            node->pfx_top = new_pfx_top;
            // UpdatePfx(node, newpfx);
        }
        delete expand;
        delete merge;
    }
}
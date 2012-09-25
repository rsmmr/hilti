

void init_result {
    prev = 0;
    acceptid = -1;
    accept_offset = 0;
    ptr = dfa_initial;
}



void start_dfa(str, len, result) {
    if (ptr == null)
        init_result();

    success = (result->cur_state_fcn)(str, len, 0, result->prev, result);

    if (success > 0) {
        accept_offset = length-accept_offset + begin;
    }
    // if offset > 0 that means we found another match, so add START_OFFSET
    // otherwise, we didn't find another match
    // LETS NOT WORRY ABOUT THIS NOW AND JUST USE NORMAL OFFSETS

    return success;
}

// increment str by using GEP to get next element???? instead of offset



int dfa_state_x(char* str, int len, char last?, assert first, assert last, struct result)
{
    // if accept state
    result->accept_id = ID;
    result->accept_offset = len; // this gets subtracted from total string length at the top level to get actual offset

    // if offset == len
    // set this fcn as the cur state
    // set prev to last_cp
    // return last_accept. this might be -1 to indicate partial match

    // current cp = *str;
    //str++;
    //len--;

    // if have assertions
    int asserts = NO_ASSETIONS;
    if (offset = 0)
        asserts |= first;
    if (offset == len-1)
        asserts |= last;
    // check last

    // if have transitions
    char cur = str[offset];
    if (ccl_x(cur, assets))
        return dfa_state_y(...);

    // no further transition possible,
    // set cur state to JAMMED
    // return last_accept id OR, if last_accept is -1 , return 0
    //  to indicate failure (since we can't transition past this point)  [ use SELECT instruction ]
}

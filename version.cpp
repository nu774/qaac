const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.22";
#else
    return "1.11";
#endif
}

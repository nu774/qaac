const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.11";
#else
    return "2.11";
#endif
}

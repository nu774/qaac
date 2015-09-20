const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.54";
#else
    return "2.54";
#endif
}

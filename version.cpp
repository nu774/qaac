const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.24";
#else
    return "2.24";
#endif
}

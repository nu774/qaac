const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.26";
#else
    return "2.26";
#endif
}

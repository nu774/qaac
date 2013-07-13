const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.19";
#else
    return "2.19";
#endif
}

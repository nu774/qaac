const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.38";
#else
    return "2.38";
#endif
}

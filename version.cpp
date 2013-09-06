const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.20";
#else
    return "2.20";
#endif
}

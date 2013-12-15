const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.30";
#else
    return "2.30";
#endif
}

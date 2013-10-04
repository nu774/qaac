const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.22";
#else
    return "2.22";
#endif
}

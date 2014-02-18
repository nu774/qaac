const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.35";
#else
    return "2.35";
#endif
}

const char *get_qaac_version()
{
#ifdef REFALAC
    return "1.14";
#else
    return "2.14";
#endif
}
